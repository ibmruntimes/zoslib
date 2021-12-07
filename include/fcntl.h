///////////////////////////////////////////////////////////////////////////////
// Licensed Materials - Property of IBM
// ZOSLIB
// (C) Copyright IBM Corp. 2021. All Rights Reserved.
// US Government Users Restricted Rights - Use, duplication
// or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
///////////////////////////////////////////////////////////////////////////////

#ifndef ZOS_FCNTL_H_
#define ZOS_FCNTL_H_

#if defined(ZOSLIB_OVERRIDE_CLIB) || defined(ZOSLIB_OVERRIDE_CLIB_FCNTL)

#undef open
#define open __open_replaced
#include_next <fcntl.h>
#undef open

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * Same as C open but tags new files as ASCII (819)
 */
int __open_ascii(const char *filename, int opts, ...);
int open(const char *filename, int opts, ...) asm("__open_ascii");

#if defined(__cplusplus)
};
#endif

#else // #if !(defined(ZOSLIB_OVERRIDE_CLIB) || defined(ZOSLIB_OVERRIDE_CLIB_FCNTL))

#include_next <fcntl.h>

#endif

#endif
