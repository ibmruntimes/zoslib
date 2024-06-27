///////////////////////////////////////////////////////////////////////////////
// Licensed Materials - Property of IBM
// ZOSLIB
// (C) Copyright IBM Corp. 2021. All Rights Reserved.
// US Government Users Restricted Rights - Use, duplication
// or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
///////////////////////////////////////////////////////////////////////////////

#ifndef ZOS_PTHREAD_H_
#define ZOS_PTHREAD_H_

#define __XPLAT 1
#include "zos-macros.h"
#include <sys/types.h>


#if defined(__cplusplus)
extern "C" {
#endif
__Z_EXPORT int __pthread_create_extended(pthread_t *thread,
                          const pthread_attr_t *attr,
                          void *(*start_routine)(void *),
                          void *arg); 
#if defined(__cplusplus)
}
#endif

#if defined(ZOSLIB_OVERRIDE_CLIB) || defined(ZOSLIB_OVERRIDE_CLIB_PTHREAD)
#define pthread_create __pthread_create_replaced
#endif

#include_next <pthread.h>

#if defined(ZOSLIB_OVERRIDE_CLIB) || defined(ZOSLIB_OVERRIDE_CLIB_PTHREAD)

#if defined(__cplusplus)
extern "C" {
#endif

#undef pthread_create 
__Z_EXPORT int pthread_create(pthread_t *thread,
                          const pthread_attr_t *attr,
                          void *(*start_routine)(void *),
                          void *arg) asm("__pthread_create_extended");

#if defined(__cplusplus)
}
#endif
#endif /* defined(ZOSLIB_OVERRIDE_CLIB) || defined(ZOSLIB_OVERRIDE_CLIB_PTHREAD) */

#ifndef PTHREAD_RWLOCK_INITIALIZER
#define PTHREAD_RWLOCK_INITIALIZER PTHREAD_RWLOCK_INITIALIZER_NP
#endif

#endif
