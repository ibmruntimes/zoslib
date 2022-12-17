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

#undef realpath
#define realpath __realpath_replaced
#undef mkstemp
#define mkstemp __mkstemp_replaced
#include_next <stdlib.h>
#undef mkstemp
#undef realpath

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * Same as original realpath, but this allocates a buffer if second parm is NULL as defined in Posix.1-2008
 */
__Z_EXPORT char *realpath(const char * __restrict__, char * __restrict__) asm("__realpath_extended");
/**
 * Same as C mkstemp but tags fd as ASCII (819)
 */
__Z_EXPORT int mkstemp(char*) asm("__mkstemp_ascii");

#if defined(__cplusplus)
}
#endif
#else
#include_next <stdlib.h>
#endif

#endif
