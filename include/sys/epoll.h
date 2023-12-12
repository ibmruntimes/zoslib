///////////////////////////////////////////////////////////////////////////////
// Licensed Materials - Property of IBM
// ZOSLIB
// (C) Copyright IBM Corp. 2020. All Rights Reserved.
// US Government Users Restricted Rights - Use, duplication
// or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
///////////////////////////////////////////////////////////////////////////////

#ifndef ZOS_SYS_EPOLL_H_
#define ZOS_SYS_EPOLL_H_

#define __XPLAT 1

#include "zos-macros.h"

#if (__EDC_TARGET < 0x42050000) && defined(ZOSLIB_ENABLE_V2R5_FEATURES)

#include <sys/types.h>
#include <stdint.h>

/* epoll_create options */
/* EPOLL_CLOEXEC: same value as O_CLOEXEC in fcntl.h */
#define EPOLL_CLOEXEC    0x00001000

/* epoll_ctl options */
#define EPOLL_CTL_ADD    0
#define EPOLL_CTL_MOD    1
#define EPOLL_CTL_DEL    2

/* epoll events */
#define EPOLLRDNORM     0x0001
#define EPOLLRDBAND     0x0002
#define EPOLLIN         0x0003
#define EPOLLOUT        0x0004
#define EPOLLWRBAND     0x0008
#define EPOLLPRI        0x0010
#define EPOLLERR        0x0020
#define EPOLLHUP        0x0040
#define EPOLLEXCLUSIVE  0x20000000
#define EPOLLONESHOT    0x40000000

#if defined(__cplusplus)
extern "C" {
#endif

typedef union epoll_data {
    void *      ptr;
    int         fd;
    uint32_t    u32;
    uint64_t    u64;
} epoll_data_t;

#if defined(__clang__) && !defined(__ibmxl__)
struct epoll_event {
    uint32_t        events;
    epoll_data_t    data __attribute__((packed));
};
#else
#pragma pack(1)
struct epoll_event {
    uint32_t        events;
    epoll_data_t    data;
};
#pragma pack(reset)
#endif

__Z_EXPORT extern int (*epoll_create)(int);
__Z_EXPORT extern int (*epoll_create1)(int);
__Z_EXPORT extern int (*epoll_ctl)(int, int, int, struct epoll_event *);
__Z_EXPORT extern int (*epoll_wait)(int, struct epoll_event *, int, int);
__Z_EXPORT extern int (*epoll_pwait)(int, struct epoll_event *, int, int, const sigset_t *);

#if defined(__cplusplus)
}
#endif

#endif //!(__EDC_TARGET < 0x42050000)  && defined(ZOSLIB_ENABLE_V2R5_FEATURES)

#endif
