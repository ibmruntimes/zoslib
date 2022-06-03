///////////////////////////////////////////////////////////////////////////////
// Licensed Materials - Property of IBM
// ZOSLIB
// (C) Copyright IBM Corp. 2020. All Rights Reserved.
// US Government Users Restricted Rights - Use, duplication
// or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
///////////////////////////////////////////////////////////////////////////////

#include "zos-semaphore.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/modes.h>

static int returnStatus(int error, const char *msg) {
  if (0 != error) {
    if (msg)
      perror(msg);
    errno = error;
    return -1;
  }
  return 0;
}

unsigned int __zsync_val_compare_and_swap32(volatile unsigned int *__p,
                                            unsigned int __compVal,
                                            unsigned int __exchVal) {
  unsigned int initv;
  __asm(" cs  %1,%3,%2 \n "
        " lgr %0,%1 \n"
        : "=r"(initv), "+r"(__compVal), "+m"(*__p)
        : "r"(__exchVal)
        :);
  return initv;
}

static unsigned int atomic_helper(volatile unsigned int *loc, int val) {
  volatile unsigned int tmp = *loc;
  volatile unsigned int org;
  org = __zsync_val_compare_and_swap32(loc, tmp, tmp + val);
  while (org != tmp) {
    tmp = *loc;
    org = __zsync_val_compare_and_swap32(loc, tmp, tmp + val);
  }
  return org;
}

unsigned int atomic_dec(volatile unsigned int *loc) {
  return atomic_helper(loc, -1);
}

unsigned int atomic_inc(volatile unsigned int *loc) {
  return atomic_helper(loc, 1);
}

int __sem_init(__sem_t *s0, int shared, unsigned int val) { // no lock
  ____sem_t *s = (____sem_t *)calloc(1, sizeof(____sem_t));
  int err;
  s0->_s = s;
  s->value = val;
  if (shared) {
    s->id = getpid();
  } else {
    s->id = 0;
  }
  if (shared == 0) {
    int rc;
    s->waitcnt = 0;
    rc = pthread_mutex_init(&s->mutex, 0);
    if (rc)
      err = errno;
    if (0 == rc) {
      rc = pthread_cond_init(&s->cond, 0);
      if (rc)
        err = errno;
      if (0 == rc)
        return 0;
      pthread_mutex_destroy(&s->mutex);
      free(s);
      s0->_s = 0;
      return returnStatus(err, "pthread_cond_init");
    } else {
      free(s);
      s0->_s = 0;
      return returnStatus(err, "pthread_mutex_init");
    }
  }
  return 0;
}

static int __sem_post_thread_w(____sem_t *s) {
  ++s->value;
  if (s->waitcnt > 0) {
    int rc = pthread_cond_signal(&s->cond);
    if (rc) {
      // unexpected error
      perror("pthread_cond_signal");
      errno = EINVAL;
      --s->value;
      return -1;
    }
  }
  return 0;
}

static int __sem_post_thread(____sem_t *s) {
  int rc, err;
  rc = pthread_mutex_lock(&s->mutex);
  if (rc == 0) {
    rc = __sem_post_thread_w(s);
    if (rc == 0) {
      pthread_mutex_unlock(&s->mutex);
      return 0;
    } else {
      err = errno;
      pthread_mutex_unlock(&s->mutex);
      return returnStatus(err, 0);
    }
  }
  return returnStatus(errno, "pthread_mutex_lock");
}

int __sem_post(__sem_t *s0) {
  ____sem_t *s = (____sem_t *)s0->_s;
  volatile unsigned int o;
  if (s->id != 0 && s->id != getpid()) {
    return returnStatus(EINVAL, 0);
  }
  if (s->id == 0) {
    int rc = __sem_post_thread(s);
    return rc;
  } else {
    atomic_inc(&s->value);
  }
  return 0;
}

static int __sem_trywait_thread_w(____sem_t *s) {
  if (s->value > 0) {
    --s->value;
    return 0;
  }
  errno = EAGAIN;
  return -1;
}

static int __sem_trywait_thread(____sem_t *s) {
  int rc, err;
  rc = pthread_mutex_lock(&s->mutex);
  if (rc == 0) {
    rc = __sem_trywait_thread_w(s);
    if (rc == 0) {
      pthread_mutex_unlock(&s->mutex);
      return 0;
    } else {
      err = errno;
      pthread_mutex_unlock(&s->mutex);
      return returnStatus(err, 0);
    }
  }
  return returnStatus(errno, "pthread_mutex_lock");
}

int __sem_trywait(__sem_t *s0) {
  ____sem_t *s = (____sem_t *)s0->_s;
  volatile unsigned int v;
  volatile unsigned int o;
  if (s->id != 0 && s->id != getpid()) {
    return returnStatus(EINVAL, 0);
  }
  if (s->id == 0) {
    int rc = __sem_trywait_thread(s);
    return rc;
  }
  v = s->value;
  if (v > 0) {
    o = __zsync_val_compare_and_swap32(&s->value, v, v - 1);
    if (o == v) {
      return 0;
    }
  }
  return returnStatus(EAGAIN, 0);
}

static void Usleep(unsigned int msec) {
  int sec = msec / 1000000;
  if (sec == 0) {
    usleep(msec);
  } else {
    sleep(sec);
    usleep(msec - (sec * 1000000));
  }
}

static int we_expired(const struct timespec *t0) {
  unsigned long long value, sec, nsec;
  __stckf(&value);
  sec = (value / 4096000000UL) - 2208988800UL;
  if (sec > t0->tv_sec) {
    return 1;
  }

  if (sec == t0->tv_sec) {
    nsec = (value % 4096000000UL) * 1000 / 4096;
    if (nsec > t0->tv_nsec) {
      return 1;
    }
  }
  return 0;
}

static int __sem_timedwait_share(____sem_t *s,
                                 const struct timespec *abs_timeout) {
  volatile unsigned int v;
  volatile unsigned int o;
  unsigned int cnt = 0;
  if (s->id != getpid()) {
    return returnStatus(EINVAL, 0);
  }
  cnt = 0;
  while (1) {
    v = s->value;
    if (v == 0) {
      if (abs_timeout && we_expired(abs_timeout)) {
        return returnStatus(ETIMEDOUT, 0);
      }
      if (cnt <= 10000000) {
        cnt += 500;
      }
      Usleep(cnt);
    } else {
      // v > 0
      o = __zsync_val_compare_and_swap32(&s->value, v, v - 1);
      if (o == v) {
        return 0;
      }
    }
  }
  return 0;
}

static int __sem_timedwait_thread_w(____sem_t *s,
                                    const struct timespec *abs_timeout) {
  int rc = 0;
  while (s->value == 0 && rc == 0) {
    ++s->waitcnt;
    if (abs_timeout)
      rc = pthread_cond_timedwait(&s->cond, &s->mutex, abs_timeout);
    else
      rc = pthread_cond_wait(&s->cond, &s->mutex);
    --s->waitcnt;
  }
  if (rc == 0 && s->value > 0)
    --s->value;
  return rc;
}

static int __sem_timedwait_thread(____sem_t *s,
                                  const struct timespec *abs_timeout) {
  int rc, err;
  rc = pthread_mutex_lock(&s->mutex);
  if (rc == 0) {
    rc = __sem_timedwait_thread_w(s, abs_timeout);
    if (rc == 0) {
      pthread_mutex_unlock(&s->mutex);
      return 0;
    } else {
      err = errno;
      pthread_mutex_unlock(&s->mutex);
      return returnStatus(err, 0);
    }
  }
  return returnStatus(errno, "pthread_mutex_lock");
}

int __sem_timedwait(__sem_t *s0, const struct timespec *abs_timeout) {
  ____sem_t *s = (____sem_t *)s0->_s;
  int rc;
  if (s->id) {
    rc = __sem_timedwait_share(s, abs_timeout);
  } else {
    rc = __sem_timedwait_thread(s, abs_timeout);
  }
  return rc;
}

int __sem_wait(__sem_t *s0) {
  ____sem_t *s = (____sem_t *)s0->_s;
  int rc = __sem_timedwait(s0, 0);
  return rc;
}

int __sem_destroy(__sem_t *s0) {
  ____sem_t *s = (____sem_t *)s0->_s;
  s->id = 0;
  s->value = 0;
  pthread_mutex_destroy(&s->mutex);
  pthread_cond_destroy(&s->cond);
  free(s);
  s0->_s = 0;
  return 0;
}
static int __sem_getvalue_thread_w(____sem_t *s, int *sval) {
  *sval = s->value;
  return 0;
}
static int __sem_getvalue_thread(____sem_t *s, int *sval) {
  int rc, err;
  rc = pthread_mutex_lock(&s->mutex);
  if (rc == 0) {
    rc = __sem_getvalue_thread_w(s, sval);
    if (rc == 0) {
      pthread_mutex_unlock(&s->mutex);
      return 0;
    } else {
      err = errno;
      pthread_mutex_unlock(&s->mutex);
      return returnStatus(err, 0);
    }
  }
  return returnStatus(errno, "pthread_mutex_lock");
}

int __sem_getvalue(__sem_t *s0, int *sval) {
  ____sem_t *s = (____sem_t *)s0->_s;
  if (s->id) {
    *sval = s->value;
  } else {
    int rc = __sem_getvalue_thread(s, sval);
    return rc;
  }
  return 0;
}
