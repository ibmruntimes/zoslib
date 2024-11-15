///////////////////////////////////////////////////////////////////////////////
// Licensed Materials - Property of IBM
// ZOSLIB
// (C) Copyright IBM Corp. 2020. All Rights Reserved.
// US Government Users Restricted Rights - Use, duplication
// or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
///////////////////////////////////////////////////////////////////////////////

#ifndef ZOS_SYS_TIME_H_
#define ZOS_SYS_TIME_H_

#define __XPLAT 1
#include "zos-macros.h"

#if (__EDC_TARGET < 0x42050000) && defined(ZOSLIB_ENABLE_V2R5_FEATURES)

#include_next <sys/time.h>

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * Changes the access and modification times of a file
 * \param [in] fd file descriptor to modify
 * \param [in] tv timeval structure containing new time
 * \return return 0 for success, or -1 for failure.
 */
__Z_EXPORT extern int (*futimes)(int fd, const struct timeval tv[2]);
/**
 * Changes the access and modification times of a file
 * \param [in] filename file path to modify
 * \param [in] tv timeval structure containing new time
 * \return return 0 for success, or -1 for failure.
 */
__Z_EXPORT extern int (*lutimes)(const char *filename, const struct timeval tv[2]);

#if defined(__cplusplus)
}
#endif

#else //!(__EDC_TARGET < 0x42050000) && defined(ZOSLIB_ENABLE_V2R5_FEATURES)
#include_next <sys/time.h>

#if defined(__cplusplus)
extern "C" {
#endif
/**
 * Changes the access and modification times of a file
 * \param [in] fd file descriptor to modify
 * \param [in] tv timeval structure containing new time
 * \return return 0 for success, or -1 for failure.
 */
__Z_EXPORT int futimes(int fd, const struct timeval tv[2]);
/**
 * Changes the access and modification times of a file
 * \param [in] filename file path to modify
 * \param [in] tv timeval structure containing new time
 * \return return 0 for success, or -1 for failure.
 */
__Z_EXPORT int lutimes(const char *filename, const struct timeval tv[2]);
#if defined(__cplusplus)
}
#endif

#endif

/* Convenience macros for operations on timevals.
   NOTE: `timercmp' does not work for >= or <=.  */
#define timerisset(tvp)        ((tvp)->tv_sec || (tvp)->tv_usec)
#define timerclear(tvp)        ((tvp)->tv_sec = (tvp)->tv_usec = 0)
#define timercmp(a, b, CMP)                                                  \
  (((a)->tv_sec == (b)->tv_sec)                                               \
   ? ((a)->tv_usec CMP (b)->tv_usec)                                          \
   : ((a)->tv_sec CMP (b)->tv_sec))
#define timeradd(a, b, result)                                               \
  do {                                                                        \
    (result)->tv_sec = (a)->tv_sec + (b)->tv_sec;                             \
    (result)->tv_usec = (a)->tv_usec + (b)->tv_usec;                          \
    if ((result)->tv_usec >= 1000000)                                         \
      {                                                                       \
        ++(result)->tv_sec;                                                   \
        (result)->tv_usec -= 1000000;                                         \
      }                                                                       \
  } while (0)
#define timersub(a, b, result)                                               \
  do {                                                                        \
    (result)->tv_sec = (a)->tv_sec - (b)->tv_sec;                             \
    (result)->tv_usec = (a)->tv_usec - (b)->tv_usec;                          \
    if ((result)->tv_usec < 0) {                                              \
      --(result)->tv_sec;                                                     \
      (result)->tv_usec += 1000000;                                           \
    }                                                                         \
  } while (0)

#endif
