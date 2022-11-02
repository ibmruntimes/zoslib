///////////////////////////////////////////////////////////////////////////////
//// Licensed Materials - Property of IBM
//// ZOSLIB
//// (C) Copyright IBM Corp. 2022. All Rights Reserved.
//// US Government Users Restricted Rights - Use, duplication
//// or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
/////////////////////////////////////////////////////////////////////////////////
//
#ifndef _ZOS_STRING_H
#define _ZOS_STRING_H

#include_next <string.h>

#ifdef __cplusplus
extern "C" {
#endif

size_t strnlen(const char *, size_t );
char *strpcpy(char *, const char *);

const char *strsignal(int );
const char *sigdescr_np(int);
const char *sigabbrev_np(int);

#ifdef __cplusplus
}
#endif

#endif
