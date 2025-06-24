///////////////////////////////////////////////////////////////////////////////
// Licensed Materials - Property of IBM
// ZOSLIB
// (C) Copyright IBM Corp. 2021. All Rights Reserved.
// US Government Users Restricted Rights - Use, duplication
// or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
///////////////////////////////////////////////////////////////////////////////

#ifndef ZOS_TIME_H_
#define ZOS_TIME_H_

#include "zos-macros.h"

#include_next <time.h>

// __clockid_t is defined in sys/types.h #if __EDC_TARGET >= __EDC_LE4205 as
#ifndef __clockid_t
  #define __clockid_t    1
  typedef unsigned  int clockid_t;
#endif

// These are defined in the system's time.h  #if __EDC_TARGET >= __EDC_LE4205
#ifndef CLOCK_REALTIME
  #define CLOCK_REALTIME        0
#endif
#ifndef CLOCK_MONOTONIC
  #define CLOCK_MONOTONIC       1
#endif
// These are not defined anywhere as of LE 3.1:
#define CLOCK_HIGHRES           2
#define CLOCK_THREAD_CPUTIME_ID 3

#if defined(__cplusplus)
extern "C" {
#endif
__Z_EXPORT int clock_gettime(clockid_t cld_id, struct timespec * tp);

/**
 * Suspends the execution of the calling thread until either at least the
 * time specified in *req has elapsed, an event occurs, or a signal arrives.
 * \param [in] req struct used to specify intervals of time with nanosecond
 *  precision
 * \param [out] rem the remaining time if the call is interrupted
 */
__Z_EXPORT int nanosleep(const struct timespec*, struct timespec*);
#if defined(__cplusplus)
}
#endif

#endif
