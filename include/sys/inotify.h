///////////////////////////////////////////////////////////////////////////////
// Licensed Materials - Property of IBM
// ZOSLIB
// (C) Copyright IBM Corp. 2020. All Rights Reserved.
// US Government Users Restricted Rights - Use, duplication
// or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
///////////////////////////////////////////////////////////////////////////////

#ifndef ZOS_SYS_INOTIFY_H_
#define ZOS_SYS_INOTIFY_H_

#define __XPLAT 1

#if (__EDC_TARGET < 0x42050000)

#include <sys/types.h>
#include <stdint.h>

/* inotify_init options */
#define IN_CLOEXEC      0x00001000
#define IN_NONBLOCK     0x00000004

/* inotify events */
#define IN_ACCESS           0x00000001
#define IN_MODIFY           0x00000002
#define IN_ATTRIB           0x00000004
#define IN_CLOSE_WRITE      0x00000008
#define IN_CLOSE_NOWRITE    0x00000010
#define IN_OPEN             0x00000020
#define IN_MOVED_FROM       0x00000040
#define IN_MOVED_TO         0x00000080
#define IN_CREATE           0x00000100
#define IN_DELETE           0x00000200
#define IN_DELETE_SELF      0x00000400
#define IN_MOVE_SELF        0x00000800

/* Events sent by the kernel */
#define IN_UNMOUNT      0x00002000
#define IN_Q_OVERFLOW   0x00004000
#define IN_IGNORED      0x00008000

/* Helper events */
#define IN_CLOSE        (IN_CLOSE_WRITE | IN_CLOSE_NOWRITE)
#define IN_MOVE         (IN_MOVED_FROM | IN_MOVED_TO)
#define IN_ALL_EVENTS   (IN_ACCESS | IN_MODIFY | IN_ATTRIB | \
                         IN_CLOSE_WRITE | IN_CLOSE_NOWRITE | \
                         IN_OPEN | IN_MOVED_FROM | \
                         IN_MOVED_TO | IN_CREATE | \
                         IN_DELETE | IN_DELETE_SELF | \
                         IN_MOVE_SELF)

/* Special flags */
#define IN_ONLYDIR      0x01000000
#define IN_DONT_FOLLOW  0x02000000
#define IN_EXCL_UNLINK  0x04000000
#define IN_MASK_CREATE  0x10000000
#define IN_MASK_ADD     0x20000000
#define IN_ISDIR        0x40000000
#define IN_ONESHOT      0x80000000

#if defined(__cplusplus)
extern "C" {
#endif

struct inotify_event {
  int         wd;
  uint32_t    mask;
  uint32_t    cookie;
  uint32_t    len;
  char        name[];
};

int (*inotify_init)(void);
int (*inotify_init1)(int);
int (*inotify_add_watch)(int, const char *,
                                    uint32_t);
int (*inotify_rm_watch)(int, int);

#if defined(__cplusplus)
};
#endif

#else //!(__EDC_TARGET < 0x42050000)
#include_next <sys/inotify.h>
#endif

#endif
