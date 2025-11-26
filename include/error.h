///////////////////////////////////////////////////////////////////////////////
// Licensed Materials - Property of IBM
// ZOSLIB
// (C) Copyright IBM Corp. 2020, 2024. All Rights Reserved.
// US Government Users Restricted Rights - Use, duplication
// or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
///////////////////////////////////////////////////////////////////////////////

#ifndef ZOS_ERROR_H_
#define ZOS_ERROR_H_

#include "zos-macros.h"

#ifdef __cplusplus
extern "C" {
#endif

__Z_EXPORT void __error(int status, int errnum, const char *format, ...);

#ifdef ZOSLIB_OVERRIDE_CLIB_ERROR
__Z_EXPORT void error(int status, int errnum, const char *format, ...) __asm("__error");
#endif

#ifdef __cplusplus
}
#endif

#endif // ZOS_ERROR_H_
