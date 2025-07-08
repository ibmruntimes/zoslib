///////////////////////////////////////////////////////////////////////////////
// Licensed Materials - Property of IBM
// ZOSLIB
// (C) Copyright IBM Corp. 2021. All Rights Reserved.
// US Government Users Restricted Rights - Use, duplication
// or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
///////////////////////////////////////////////////////////////////////////////

#ifndef ZOS_FCNTL_H_
#define ZOS_FCNTL_H_

#include "zos-macros.h"
#include <sys/types.h>

#if defined(__cplusplus)
extern "C" {
#endif
/**
 * Same as C open but tags new files as ASCII (819)
 */
__Z_EXPORT extern int __open_ascii(const char *filename, int opts, ...);
__Z_EXPORT extern int __creat_ascii(const char *filename, mode_t mode);
#if defined(__cplusplus)
}
#endif

#if defined(ZOSLIB_OVERRIDE_CLIB)

#undef open
#undef creat
#define open __open_replaced
#define creat __creat_replaced
#include_next <fcntl.h>
#undef open
#undef creat

#if defined(__cplusplus)
extern "C" {
#endif

__Z_EXPORT extern int open(const char *filename, int opts, ...) __asm("__open_ascii");
__Z_EXPORT extern int creat(const char *filename, mode_t mode) __asm("__creat_ascii");

#if defined(__cplusplus)
}
#endif

#else // #if !(defined(ZOSLIB_OVERRIDE_CLIB)

#include_next <fcntl.h>

#endif

#endif
