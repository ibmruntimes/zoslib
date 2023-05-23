///////////////////////////////////////////////////////////////////////////////
// Licensed Materials - Property of IBM
// ZOSLIB
// (C) Copyright IBM Corp. 2021. All Rights Reserved.
// US Government Users Restricted Rights - Use, duplication
// or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
///////////////////////////////////////////////////////////////////////////////

#ifndef ZOS_UTMPX_H_
#define ZOS_UTMPX_H_

#define __XPLAT 1
#include "zos-macros.h"

#if defined(__cplusplus)
extern "C" {
#endif
struct utmpx *__getutxent_ascii(void);
__Z_EXPORT int utmpxname(char *);

#if defined(__cplusplus)
}
#endif

#if defined(ZOSLIB_OVERRIDE_CLIB) || defined(ZOSLIB_OVERRIDE_CLIB_UTMPX)

#undef getutxent
#define getutxent __getutxent_replaced
#include_next <utmpx.h>
#undef getutxent

#if defined(__cplusplus)
extern "C" {
#endif

__Z_EXPORT struct utmpx *getutxent(void) __asm("__getutxent_ascii");

#if defined(__cplusplus)
}
#endif
#else
#include_next <utmpx.h>
#endif

#define UTMPX_FILE __UTMPX_FILE

#endif
