///////////////////////////////////////////////////////////////////////////////
// Licensed Materials - Property of IBM
// ZOSLIB
// (C) Copyright IBM Corp. 2022. All Rights Reserved.
// US Government Users Restricted Rights - Use, duplication
// or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
///////////////////////////////////////////////////////////////////////////////

#ifndef ZOS_SYS_SOCKET_H_
#define ZOS_SYS_SOCKET_H_

#include "zos-macros.h"

#if defined(ZOSLIB_OVERRIDE_CLIB) || defined(ZOSLIB_OVERRIDE_CLIB_SOCKET)
#define __XPLAT 1

#if defined(__cplusplus)
extern "C" {
#endif

typedef unsigned int socklen_t;
#define __socklen_t 1

#define socklen_t unsigned int
struct sockaddr {
  unsigned char sa_len;
  unsigned char sa_family;
  char          sa_data[14];    /* variable length data */
};
#define __sockaddr 1
__Z_EXPORT int __getsockname_fixed(int, struct sockaddr * __restrict__, socklen_t * __restrict__);

#if defined(__cplusplus)
};
#endif

#undef getsockname
#define getsockname __getsockname_replaced
#include_next <sys/socket.h>
#undef getsockname

__Z_EXPORT int getsockname(int, struct sockaddr * __restrict__, socklen_t * __restrict__) asm("__getsockname_fixed");

#else
#include_next <sys/socket.h>
#endif  // ZOSLIB_OVERRIDE_CLIB || ZOSLIB_OVERRIDE_CLIB_SOCKET
#endif  // ZOS_SYS_SOCKET_H_
