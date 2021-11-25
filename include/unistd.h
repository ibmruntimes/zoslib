///////////////////////////////////////////////////////////////////////////////
// Licensed Materials - Property of IBM
// ZOSLIB
// (C) Copyright IBM Corp. 2021. All Rights Reserved.
// US Government Users Restricted Rights - Use, duplication
// or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
///////////////////////////////////////////////////////////////////////////////

#ifndef ZOS_UNISTD_H_
#define ZOS_UNISTD_H_

#if defined(ZOSLIB_OVERRIDE_CLIB) || defined(ZOSLIB_OVERRIDE_CLIB_UNISTD)

#undef pipe 
#define pipe __pipe_replaced
#include_next <unistd.h>
#undef pipe

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * Same as C pipe but tags pipes as ASCII (819)
 */
int pipe(int [2]) asm("__pipe_ascii");

#if defined(__cplusplus)
};
#endif

#else // #if !(defined(ZOSLIB_OVERRIDE_CLIB) || defined(ZOSLIB_OVERRIDE_CLIB_UNISTD))
#include_next <unistd.h>
#endif

#endif
