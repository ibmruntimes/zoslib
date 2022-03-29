///////////////////////////////////////////////////////////////////////////////
// Licensed Materials - Property of IBM
// ZOSLIB
// (C) Copyright IBM Corp. 2020. All Rights Reserved.
// US Government Users Restricted Rights - Use, duplication
// or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
///////////////////////////////////////////////////////////////////////////////

#ifndef ZOS_SYS_EVENTFD_H_
#define ZOS_SYS_EVENTFD_H_

#define __XPLAT 1

#if (__EDC_TARGET < 0x42050000)
#define EFD_SEMAPHORE 0x00002000
#define EFD_CLOEXEC   0x00001000
#define EFD_NONBLOCK  0x00000004

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * create a file descriptor for event notification
 * \param [in] initval initial value for counter
 * \param [in] flags behaviour flags for event fd
 * \return returns a fd for success, or -1 for failure.
 */
extern int (*eventfd)(unsigned int initval, int flags);

#if defined(__cplusplus)
};
#endif

#else //!(__EDC_TARGET < 0x42050000)
#include_next <sys/eventfd.h>
#endif

#endif
