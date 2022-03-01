///////////////////////////////////////////////////////////////////////////////
// Licensed Materials - Property of IBM
// ZOSLIB
// (C) Copyright IBM Corp. 2020. All Rights Reserved.
// US Government Users Restricted Rights - Use, duplication
// or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
///////////////////////////////////////////////////////////////////////////////

// APIs that implement POSIX semaphores.

#ifndef ZOS_SEMAPHORE_H_
#define ZOS_SEMAPHORE_H_

#include <errno.h>
#include <inttypes.h>
#include <pthread.h>
#include <sys/sem.h>

#if __WORDSIZE == 64
#define __SIZEOF_SEM_T 32
#else
#define __SIZEOF_SEM_T 16
#endif

#define SEM_FAILED ((sem_t *)0)

typedef struct {
  pthread_mutex_t mutex;
  pthread_cond_t cond;
  unsigned int value;
} sem_t;

typedef struct __sem {
  volatile unsigned int value;
  volatile unsigned int id; // 0 for non shared (thread), pid for share
  volatile unsigned int waitcnt;
  pthread_mutex_t mutex;
  pthread_cond_t cond;
} ____sem_t;

typedef struct {
  ____sem_t *_s;
} __sem_t;

#ifdef __cplusplus
extern "C" {
#endif

int sem_init(sem_t *semid, int pshared, unsigned int value);

int sem_destroy(sem_t *semid);

int sem_wait(sem_t *semid);

int sem_trywait(sem_t *semid);

int sem_post(sem_t *semid);

int sem_timedwait(sem_t *semid, const struct timespec *timeout);

/**TODO(itodorov) - zos: document these interfaces**/
int __sem_init(__sem_t *s0, int shared, unsigned int val);
int __sem_post(__sem_t *s0);
int __sem_trywait(__sem_t *s0);
int __sem_timedwait(__sem_t *s0, const struct timespec *abs_timeout);
int __sem_wait(__sem_t *s0);
int __sem_getvalue(__sem_t *s0, int *sval);
int __sem_destroy(__sem_t *s0);

unsigned int atomic_dec(volatile unsigned int *loc);
unsigned int atomic_inc(volatile unsigned int *loc);

#ifdef __cplusplus
}
#endif

#endif // ZOS_SEMAPHORE_H_
