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
#include <features.h>


#if defined(__cplusplus)
extern "C" {
#endif
__Z_EXPORT char *__realpath_extended(const char * __restrict__, char * __restrict__);
#ifdef __NATIVE_ASCII_F
__Z_EXPORT int __mkstemp_ascii(char*);
#endif
#if defined(__cplusplus)
}
#endif

#if defined(ZOSLIB_OVERRIDE_CLIB) || defined(ZOSLIB_OVERRIDE_CLIB_STDLIB)
/* Modify function names in header to avoid conflict with new prototypes */
#undef realpath
#define realpath __realpath_replaced
#undef mkstemp
#define mkstemp __mkstemp_replaced
#endif

#if defined(ZOSLIB_OVERRIDE_CLIB_GETENV) && defined(__NATIVE_ASCII_F)
#undef getenv
#define getenv __getenv_replaced
#endif


#include_next <stdlib.h>

#if defined(ZOSLIB_OVERRIDE_CLIB) || defined(ZOSLIB_OVERRIDE_CLIB_STDLIB)

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * Same as original realpath, but this allocates a buffer if second parm is NULL as defined in Posix.1-2008
 */
#undef realpath
__Z_EXPORT char *realpath(const char * __restrict__, char * __restrict__) __asm("__realpath_extended");

#ifdef __NATIVE_ASCII_F
/**
 * Same as C mkstemp but tags fd as ASCII (819)
 */
#undef mkstemp
__Z_EXPORT int mkstemp(char*) __asm("__mkstemp_ascii");
#endif /* __NATIVE_ASCII_F */

#if defined(__cplusplus)
}
#endif
#endif /* defined(ZOSLIB_OVERRIDE_CLIB) || defined(ZOSLIB_OVERRIDE_CLIB_STDLIB) */

#if defined(ZOSLIB_OVERRIDE_CLIB_GETENV) && defined(__NATIVE_ASCII_F)
#undef getenv

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * Replace getenv with the ascii implementation of __getenv (@@A00423) 
   which copies pointer to a buffer and is retained even after the environment changes
 */
__Z_EXPORT char* getenv(const char*) __asm("@@A00423");
#if defined(__cplusplus)
}
#endif
#endif

#if defined(__cplusplus)
extern "C" {
#endif
/**
 * C Lib functions that do not conflict with z/OS LE
 */
__Z_EXPORT int getloadavg(double loadavg[], int nelem);
__Z_EXPORT const char * getprogname(void);
#if defined(__cplusplus)
}
#endif

#if defined(__cplusplus)
extern "C" {
#endif

__Z_EXPORT char *mkdtemp(char *);
#ifdef __NATIVE_ASCII_F
  __Z_EXPORT char *mkdtemp(char *) __asm("__mkdtemp_a");
#ifdef __AE_BIMODAL_F
  __Z_EXPORT char *__mkdtemp_a(char *);
  __Z_EXPORT char *__mkdtemp_e(char *);
#endif
#else
  __Z_EXPORT char *mkdtemp(char *)  __asm("__mkdtemp_e");
#endif

#if defined(__cplusplus)
}
#endif

#endif
