///////////////////////////////////////////////////////////////////////////////
// Licensed Materials - Property of IBM
// ZOSLIB
// (C) Copyright IBM Corp. 2020. All Rights Reserved.
// US Government Users Restricted Rights - Use, duplication
// or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
///////////////////////////////////////////////////////////////////////////////

#ifndef ZOS_SYS_SOCKET_H_
#define ZOS_SYS_SOCKET_H_

#define __XPLAT 1

#if (__EDC_TARGET < 0x42050000)

#include <sys/types.h>
#include_next <sys/socket.h>

/* epoll_ctl options */
#define SOCK_CLOEXEC  0x00001000
#define SOCK_NONBLOCK 16

#if defined(__cplusplus)
extern "C" {
#endif

int (*accept4)(int s, struct sockaddr * addr,
               socklen_t * addrlen, int flags);

#if defined(__cplusplus)
};
#endif

#else //!(__EDC_TARGET < 0x42050000)
#include_next <sys/socket.h>
#endif

#endif
