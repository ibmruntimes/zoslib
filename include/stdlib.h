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
#ifndef __ibmxl__
#include <locale.h>
#endif


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
#undef malloc
#define malloc __malloc_replaced
#undef free
#define free __free_replaced
#endif

#if defined(ZOSLIB_OVERRIDE_CLIB_GETENV) && defined(__NATIVE_ASCII_F)
#undef getenv
#define getenv __getenv_replaced
#endif


#include_next <stdlib.h>

#if defined(ZOSLIB_OVERRIDE_CLIB) || defined(ZOSLIB_OVERRIDE_CLIB_STDLIB)

#undef realpath
#undef mkstemp
#undef malloc
#undef free

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * Same as original realpath, but this allocates a buffer if second parm is NULL as defined in Posix.1-2008
 */
#undef realpath
__Z_EXPORT char *realpath(const char * __restrict__, char * __restrict__) __asm("__realpath_extended");
__Z_EXPORT void* malloc(size_t size) __asm("__zoslib_malloc");
__Z_EXPORT void free(void* ptr) __asm("__zoslib_free");

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

#if !defined(__ibmxl__) && (defined(ZOSLIB_OVERRIDE_CLIB) || defined(ZOSLIB_OVERRIDE_CLIB_LOCALE)) && !defined(ZOSLIB_USE_CLIB_LOCALE)
#if defined(__cplusplus)
extern "C" {
#endif
__Z_EXPORT double strtod_l(const char * __restrict__ , char ** __restrict__ , locale_t );
#if defined(__cplusplus)
}
#endif
#endif

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
__Z_EXPORT int mkostemp(char *, int flags);
__Z_EXPORT int mkstemps(char *, int suffixlen);
__Z_EXPORT int mkostemps(char *, int suffixlen, int flags);
__Z_EXPORT void *reallocarray(void *ptr, size_t nmemb, size_t size);
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
__Z_EXPORT void __zoslib_free(void* ptr);
__Z_EXPORT void* __zoslib_malloc(size_t size);
}
#endif

#endif
