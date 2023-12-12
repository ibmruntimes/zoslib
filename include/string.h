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

#include_next <string.h>

#ifdef __cplusplus
extern "C" {
#endif

__Z_EXPORT size_t strnlen(const char *, size_t );
__Z_EXPORT char *strpcpy(char *, const char *);

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
