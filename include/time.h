///////////////////////////////////////////////////////////////////////////////
// Licensed Materials - Property of IBM
// ZOSLIB
// (C) Copyright IBM Corp. 2021. All Rights Reserved.
// US Government Users Restricted Rights - Use, duplication
// or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
///////////////////////////////////////////////////////////////////////////////

#ifndef ZOS_TIME_H_
#define ZOS_TIME_H_

#define __XPLAT 1
#include "zos-macros.h"

#if (__EDC_TARGET < 0x42050000) && defined(ZOSLIB_ENABLE_V2R5_FEATURES)
#include_next <time.h>

#if defined(__cplusplus)
extern "C" {
#endif

typedef enum {
  CLOCK_REALTIME,
  CLOCK_MONOTONIC,
  CLOCK_HIGHRES,
  CLOCK_THREAD_CPUTIME_ID
} clockid_t;

/**
 * Retrieves the time of the specified clock id
 * \param [in] clk_id clock id.
 * \param [out] tp structure to store the current time to.  
 * \return return 0 for success, or -1 for failure.
 */
__Z_EXPORT extern int (*clock_gettime)(clockid_t cld_id, struct timespec * tp);
__Z_EXPORT extern int (*nanosleep)(const struct timespec*, struct timespec*);
#if defined(__cplusplus)
}
#endif

#else //!(__EDC_TARGET < 0x42050000) && defined(ZOSLIB_ENABLE_V2R5_FEATURES)

#include_next <time.h>

typedef enum {
  CLOCK_REALTIME,
  CLOCK_MONOTONIC,
  CLOCK_HIGHRES,
  CLOCK_THREAD_CPUTIME_ID
} clockid_t;

#if defined(__cplusplus)
extern "C" {
#endif
__Z_EXPORT int clock_gettime(clockid_t cld_id, struct timespec * tp);
#if defined(__cplusplus)
}
#endif

#if !defined(__cplusplus)
/*
 * ^ because the compiler's include/c++/v1/__threading_support has a call to
 * nanosleep() which is defined in include/c++/v1/__support/ibm/nanosleep.h.
*/
/**
 * Suspends the execution of the calling thread until either at least the
 * time specified in *req has elapsed, an event occurs, or a signal arrives.
 * \param [in] req struct used to specify intervals of time with nanosecond
 *  precision
 * \param [out] rem the remaining time if the call is interrupted
 */
__Z_EXPORT int nanosleep(const struct timespec*, struct timespec*);
#endif

#endif
#endif
