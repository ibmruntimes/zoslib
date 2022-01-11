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

#if (__EDC_TARGET < 0x42050000)

#include <sys/types.h>
#include <stdint.h>

/* epoll_create options */
#define EPOLL_CLOEXEC    1

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
            uint32_t        events __attribute__((__aligned__(4)));
            epoll_data_t    data;
        };
#else
        #pragma pack(4)
        struct epoll_event {
            uint32_t        events;
            epoll_data_t    data;
        };
        #pragma pack(reset)
#endif

int (*epoll_create)(int);
int (*epoll_create1)(int);
int (*epoll_ctl)(int, int, int, struct epoll_event *);
int (*epoll_wait)(int, struct epoll_event *, int, int);
int (*epoll_pwait)(int, struct epoll_event *, int, int, const sigset_t *);

#if defined(__cplusplus)
};
#endif

#else //!(__EDC_TARGET < 0x42050000)
#include_next <sys/epoll.h>
#endif

#endif
