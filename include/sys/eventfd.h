///////////////////////////////////////////////////////////////////////////////
// Licensed Materials - Property of IBM
// ZOSLIB
// (C) Copyright IBM Corp. 2020. All Rights Reserved.
// US Government Users Restricted Rights - Use, duplication
// or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
///////////////////////////////////////////////////////////////////////////////

#ifndef ZOS_TIME_H_
#define ZOS_TIME_H_

#if (__EDC_TARGET < 0x42050000)
#define EFD_SEMAPHORE 0x00002000
#define EFD_CLOEXEC   0x00001000
#define EFD_NONBLOCK  0x00000004

extern int (*eventfd)(unsigned int, int);
#else
#include_next <sys/time.h>
#endif

#endif
