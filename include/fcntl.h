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

#define __XPLAT 1

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

#if (__EDC_TARGET < 0x42050000) && defined(ZOSLIB_ENABLE_V2R5_FEATURES)
#define O_CLOEXEC   0x00001000
#define O_DIRECT    0x00002000
#define O_NOFOLLOW  0x00004000
#define O_DIRECTORY 0x00008000
#define O_PATH      0x00080000
#endif

#endif
