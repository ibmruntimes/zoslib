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
#include "zos-macros.h"

#if defined(__cplusplus)
extern "C" {
#endif
__Z_EXPORT int __socketpair_ascii(int domain, int type, int protocol, int sv[2]);
#if defined(__cplusplus)
};
#endif

#if (defined(ZOSLIB_OVERRIDE_CLIB) || defined(ZOSLIB_OVERRIDE_CLIB_SOCKET)) && defined(ZOSLIB_ENABLE_V2R5_FEATURES)
#undef socketpair
#define socketpair __socketpair_replaced
#include_next <sys/socket.h>
#undef socketpair
__Z_EXPORT int socketpair(int domain, int type, int protocol, int sv[2]) asm("__socketpair_ascii");
#else
#include_next <sys/socket.h>
#endif

#if (__EDC_TARGET < 0x42050000) && defined(ZOSLIB_ENABLE_V2R5_FEATURES)

#include <sys/types.h>

/* epoll_ctl options */
#define SOCK_CLOEXEC  0x00001000
#define SOCK_NONBLOCK 16

#if defined(__cplusplus)
extern "C" {
#endif

__Z_EXPORT extern int (*accept4)(int s, struct sockaddr * addr,
               socklen_t * addrlen, int flags);

#if defined(__cplusplus)
};
#endif
#endif

#endif
