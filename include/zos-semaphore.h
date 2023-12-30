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

#include "zos-macros.h"

#include <errno.h>
#include <inttypes.h>
#include <pthread.h>
#include <sys/sem.h>

#if defined(__WORDSIZE) && __WORDSIZE == 64
#define __SIZEOF_SEM_T 32
#else
#define __SIZEOF_SEM_T 16
#endif

#define SEM_FAILED ((sem_t *)0)

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

#define sem_t         __sem_t

#define sem_init      __sem_init
#define sem_post      __sem_post
#define sem_trywait   __sem_trywait
#define sem_timedwait __sem_timedwait
#define sem_wait      __sem_wait
#define sem_getvalue  __sem_getvalue
#define sem_destroy   __sem_destroy

#ifdef __cplusplus
extern "C" {
#endif

/**TODO(itodorov) - zos: document these interfaces**/
__Z_EXPORT int __sem_init(__sem_t *s0, int shared, unsigned int val);
__Z_EXPORT int __sem_post(__sem_t *s0);
__Z_EXPORT int __sem_trywait(__sem_t *s0);
__Z_EXPORT int __sem_timedwait(__sem_t *s0, const struct timespec *abs_timeout);
__Z_EXPORT int __sem_wait(__sem_t *s0);
__Z_EXPORT int __sem_getvalue(__sem_t *s0, int *sval);
__Z_EXPORT int __sem_destroy(__sem_t *s0);

__Z_EXPORT unsigned int atomic_dec(volatile unsigned int *loc);
__Z_EXPORT unsigned int atomic_inc(volatile unsigned int *loc);

#ifdef __cplusplus
}
#endif

#endif // ZOS_SEMAPHORE_H_
