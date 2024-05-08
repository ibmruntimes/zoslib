///////////////////////////////////////////////////////////////////////////////
// Licensed Materials - Property of IBM
// ZOSLIB
// (C) Copyright IBM Corp. 2023. All Rights Reserved.
// US Government Users Restricted Rights - Use, duplication
// or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
///////////////////////////////////////////////////////////////////////////////

#ifndef ZOS_SYSFILE_H_
#define ZOS_SYSFILE_H_

#define __XPLAT 1
#include "zos-macros.h"

#if (__EDC_TARGET < 0x42050000)
#include <sys/file.h>

#define   LOCK_SH  0x01    // shared file lock
#define   LOCK_EX  0x02    // exclusive file lock
#define   LOCK_NB  0x04    // do not block when locking
#define   LOCK_UN  0x08    // unlock file

#if defined(__cplusplus)
extern "C" {
#endif
__Z_EXPORT int flock(int fd, int operation) __asm("__flock");
#if defined(__cplusplus)
}
#endif
#else
#include_next <sys/file.h>
#endif

#endif
