///////////////////////////////////////////////////////////////////////////////
// Licensed Materials - Property of IBM
// ZOSLIB
// (C) Copyright IBM Corp. 2021. All Rights Reserved.
// US Government Users Restricted Rights - Use, duplication
// or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
///////////////////////////////////////////////////////////////////////////////

#ifndef ZOS_STDLIB_H_
#define ZOS_STDLIB_H_

#define __XPLAT 1
#include "zos-macros.h"

#if defined(__cplusplus)
extern "C" {
#endif
__Z_EXPORT char *__realpath_extended(const char * __restrict__, char * __restrict__);
__Z_EXPORT int __mkstemp_ascii(char*);
#if defined(__cplusplus)
}
#endif

#if defined(ZOSLIB_OVERRIDE_CLIB) || defined(ZOSLIB_OVERRIDE_CLIB_STDLIB)

/* Modify function names in header to avoid conflict with new prototypes */
#undef realpath
#define realpath __realpath_replaced
#undef mkstemp
#define mkstemp __mkstemp_replaced
#undef getenv
#define getenv __getenv_replaced
#include_next <stdlib.h>
#undef mkstemp
#undef realpath
#undef getenv

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * Same as original realpath, but this allocates a buffer if second parm is NULL as defined in Posix.1-2008
 */
__Z_EXPORT char *realpath(const char * __restrict__, char * __restrict__) __asm("__realpath_extended");
/**
 * Same as C mkstemp but tags fd as ASCII (819)
 */
__Z_EXPORT int mkstemp(char*) __asm("__mkstemp_ascii");

/**
 * Replace getenv with the ascii implementation of __getenv (@@A00423) 
   which copies pointer to a buffer and is retained even after the environment changes
 */
__Z_EXPORT char* getenv(const char*) __asm("@@A00423");
#if defined(__cplusplus)
}
#endif
#else
#include_next <stdlib.h>
#endif

#if defined(__cplusplus)
extern "C" {
#endif
/**
 * C Lib functions that do not conflict with z/OS LE
 */
__Z_EXPORT char *mkdtemp(char *templ);
__Z_EXPORT int getloadavg(double loadavg[], int nelem);
__Z_EXPORT const char * getprogname(void);
#if defined(__cplusplus)
}
#endif

#endif
