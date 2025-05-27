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

#if defined(ZOSLIB_TRACE_ALLOCS)
#include <sys/types.h>

#if defined(__cplusplus)
extern "C" {
#endif

char *__strdup_orig(const char *) __asm("strdup");

__Z_EXPORT char * __strdup_trace(const char *, const char *pfname, int linenum);
__Z_EXPORT char * __strndup_trace(const char *, size_t n, const char *pfname, int linenum);

#if defined(__cplusplus)
}
#endif

#define __strdup_prtsrc(x)    __strdup_trace(x, __FILE__, __LINE__)
#define __strndup_prtsrc(x,n) __strndup_trace(x, n, __FILE__, __LINE__)

#undef strdup
#undef strndup

#define strdup  __strdup_replaced
#define strndup __strndup_replaced

#endif // ZOSLIB_TRACE_ALLOCS

#include_next <string.h>

#if defined(ZOSLIB_TRACE_ALLOCS)
#undef strdup
#undef strndup
#define strdup  __strdup_prtsrc
#define strndup __strndup_prtsrc
#endif

#ifdef __cplusplus
extern "C" {
#endif

__Z_EXPORT size_t strnlen(const char *, size_t );
__Z_EXPORT char *strpcpy(char *, const char *);
#if !defined(ZOSLIB_TRACE_ALLOCS)
__Z_EXPORT char *strndup(const char *s, size_t n);
#endif

__Z_EXPORT char *strsignal(int );
__Z_EXPORT const char *sigdescr_np(int);
__Z_EXPORT const char *sigabbrev_np(int);

// Linux includes strings.h in string.h, this avoids the 
// warning - implicitly declaring library function 'strcasecmp'
// which also causes it to pick up the EBCDIC definition
#include <strings.h>

#ifdef __cplusplus
}
#endif

#endif
