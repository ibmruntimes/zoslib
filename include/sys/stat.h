///////////////////////////////////////////////////////////////////////////////
////// Licensed Materials - Property of IBM
////// ZOSLIB
////// (C) Copyright IBM Corp. 2022. All Rights Reserved.
////// US Government Users Restricted Rights - Use, duplication
////// or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
///////////////////////////////////////////////////////////////////////////////////

#ifndef ZOS_SYS_STAT_H
#define ZOS_SYS_STAT_H

#include "zos-macros.h"

typedef int mode_t;

#if defined(__cplusplus)
extern "C" {
#endif
/**
 * Same as C mkfifo but tags FIFO special files as ASCII (819)
 */
__Z_EXPORT extern int __mkfifo_ascii(const char *pathname, mode_t mode);

#if defined(__cplusplus)
};
#endif

#if defined(ZOSLIB_OVERRIDE_CLIB)

#undef mkfifo
#define mkfifo __mkfifo_replaced
#include_next <sys/stat.h>
#undef mkfifo 

#if defined(__cplusplus)
extern "C" {
#endif

__Z_EXPORT extern int mkfifo(const char *pathname, mode_t mode) __asm("__mkfifo_ascii");

#if defined(__cplusplus)
};
#endif

#else // #if !(defined(ZOSLIB_OVERRIDE_CLIB)

#include_next <sys/stat.h>

#endif

#ifdef S_TYPEISMQ  
#undef S_TYPEISMQ
#undef S_TYPEISSEM
#undef S_TYPEISSHM 
#define S_TYPEISMQ(__x)   (0) /* Test for a message queue */
#define S_TYPEISSEM(__x)  (0) /* Test for a semaphore     */
#define S_TYPEISSHM(__x)  (0) /* Test for a shared memory */
#endif

#endif
