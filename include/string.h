///////////////////////////////////////////////////////////////////////////////
//// Licensed Materials - Property of IBM
//// ZOSLIB
//// (C) Copyright IBM Corp. 2022. All Rights Reserved.
//// US Government Users Restricted Rights - Use, duplication
//// or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
/////////////////////////////////////////////////////////////////////////////////
//
#ifndef ZOS_STRING_H
#define ZOS_STRING_H

#include "zos-macros.h"

#if defined(ZOSLIB_OVERRIDE_CLIB)
#undef strerror
#undef strerror_r
#undef strnlen 
#undef strpcpy
#undef strsignal
#define strnlen strnlen_replaced
#define strpcpy strpcpy_replaced
#define strsignal strsignal_replaced
#define strerror strerror_replaced
#define strerror_r strerror_r_replaced

#include_next <string.h>
#undef strerror
#undef strerror_r
#undef strnlen
#undef strpcpy
#undef strsignal

#ifdef __cplusplus
extern "C" {
#endif
__Z_EXPORT char *strerror(int err) __asm("__strerror_ascii");
__Z_EXPORT int strerror_r(int err, char * buf, size_t buflen) __asm("__strerror_r_ascii");
#if defined(__cplusplus)
}
#endif
#else
#include_next <string.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif


/* TODO: remove these once the z/OS APAR that enables them becomes more mainstream */
__Z_EXPORT size_t strnlen(const char *, size_t );
__Z_EXPORT char *strpcpy(char *, const char *);
__Z_EXPORT char *strsignal(int );

__Z_EXPORT char *strndup(const char *s, size_t n);
__Z_EXPORT const char *sigdescr_np(int);
__Z_EXPORT const char *sigabbrev_np(int);
__Z_EXPORT void *memmem(const void *haystack, size_t haystacklen, const void *needle, size_t needlelen);
__Z_EXPORT int strverscmp(const char *l0, const char *r0);
__Z_EXPORT char *strcasestr(const char *haystack, const char *needle);

// Linux includes strings.h in string.h, this avoids the 
// warning - implicitly declaring library function 'strcasecmp'
// which also causes it to pick up the EBCDIC definition
#include <strings.h>

#ifdef __cplusplus
}
#endif

#endif
