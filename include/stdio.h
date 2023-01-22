///////////////////////////////////////////////////////////////////////////////
// Licensed Materials - Property of IBM
// ZOSLIB
// (C) Copyright IBM Corp. 2021. All Rights Reserved.
// US Government Users Restricted Rights - Use, duplication
// or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
///////////////////////////////////////////////////////////////////////////////

#ifndef ZOS_STDIO_H_
#define ZOS_STDIO_H_

#include "zos-macros.h"

#define __XPLAT 1

typedef struct __ffile FILE;

#if defined(__cplusplus)
extern "C" {
#endif
/**
 * Same as C open but tags new files as ASCII (819)
 */
__Z_EXPORT extern FILE *__fopen_ascii(const char *filename, const char *mode);

#if defined(__cplusplus)
};
#endif

#if defined(ZOSLIB_OVERRIDE_CLIB)

#undef fopen
#define fopen __fopen_replaced
#include_next <stdio.h>
#undef fopen

#if defined(__cplusplus)
extern "C" {
#endif

__Z_EXPORT extern FILE *fopen(const char *filename, const char *mode) asm("__fopen_ascii");

#if defined(__cplusplus)
};
#endif

#else // #if !(defined(ZOSLIB_OVERRIDE_CLIB)

#include_next <stdio.h>

#endif

#endif
