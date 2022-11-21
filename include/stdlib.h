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
__Z_EXPORT int __mkstemp_ascii(char*);
#if defined(__cplusplus)
};
#endif

#if defined(ZOSLIB_OVERRIDE_CLIB) || defined(ZOSLIB_OVERRIDE_CLIB_STDLIB)

#undef mkstemp
#define mkstemp __mkstemp_replaced
#include_next <stdlib.h>
#undef mkstemp 

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * Same as C mkstemp but tags fd as ASCII (819)
 */
__Z_EXPORT int mkstemp(char*) asm("__mkstemp_ascii");

#if defined(__cplusplus)
};
#endif
#else
#include_next <stdlib.h>
#endif

#endif
