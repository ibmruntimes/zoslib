///////////////////////////////////////////////////////////////////////////////
////// Licensed Materials - Property of IBM
////// ZOSLIB
////// (C) Copyright IBM Corp. 2022. All Rights Reserved.
////// US Government Users Restricted Rights - Use, duplication
////// or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
///////////////////////////////////////////////////////////////////////////////////

#ifndef ZOS_SYS_UIO_H
#define ZOS_SYS_UIO_H

#include "zos-macros.h"

#if defined(__cplusplus)
extern "C" {
#endif
/**
 * 
 */
__Z_EXPORT extern ssize_t __writev_ascii(int fd, const struct iovec *iov, int iovcnt);

#if defined(__cplusplus)
}
#endif

#if defined(ZOSLIB_OVERRIDE_CLIB)

#undef writev
#define writev __writev_replaced
#include_next <sys/uio.h>
#undef writev

#if defined(__cplusplus)
extern "C" {
#endif

__Z_EXPORT extern ssize_t writev(int fd, const struct iovec *iov, int iovcnt) __asm("__writev_ascii");

#if defined(__cplusplus)
}
#endif

#else // #if !(defined(ZOSLIB_OVERRIDE_CLIB)

#include_next <sys/uio.h>

#endif

#endif
