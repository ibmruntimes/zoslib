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
}
#endif

// Workaround issue with getsockname in LE prior to 3.1:
typedef unsigned int socklen_t;
#define __socklen_t 1

#define socklen_t unsigned int
struct sockaddr {
  unsigned char sa_len;
  unsigned char sa_family;
  char          sa_data[14];    /* variable length data */
};
// For the system's socket.h to not redefine the above sockaddr struct:
#define __sockaddr 1
#if defined(__cplusplus)
extern "C" {
#endif
__Z_EXPORT int __getsockname_fixed(int, struct sockaddr * __restrict__, socklen_t * __restrict__);
#if defined(__cplusplus)
}
#endif

#undef getsockname
#define getsockname __getsockname_replaced

#if (defined(ZOSLIB_OVERRIDE_CLIB) || defined(ZOSLIB_OVERRIDE_CLIB_SOCKET)) && defined(ZOSLIB_ENABLE_V2R5_FEATURES)
#undef socketpair
#define socketpair __socketpair_replaced
#endif

#include_next <sys/socket.h>

#undef getsockname
#if defined(__cplusplus)
extern "C" {
#endif
__Z_EXPORT extern int (*getsockname)(int, struct sockaddr * __restrict__, socklen_t * __restrict__);

#if (defined(ZOSLIB_OVERRIDE_CLIB) || defined(ZOSLIB_OVERRIDE_CLIB_SOCKET)) && defined(ZOSLIB_ENABLE_V2R5_FEATURES)
#undef socketpair
__Z_EXPORT int socketpair(int domain, int type, int protocol, int sv[2]) __asm("__socketpair_ascii");
#endif
#if defined(__cplusplus)
}
#endif

#if (__EDC_TARGET < 0x42050000) && defined(ZOSLIB_ENABLE_V2R5_FEATURES)

#include <sys/types.h>

/* epoll_ctl options */
#define SOCK_CLOEXEC  0x00001000
#define SOCK_NONBLOCK 16

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _OE_SOCKETS
__Z_EXPORT extern int (*accept4)(int s, struct sockaddr * addr,
               socklen_t * addrlen, int flags);
#endif

#if defined(__cplusplus)
}
#endif
#endif

#endif
