///////////////////////////////////////////////////////////////////////////////
// Licensed Materials - Property of IBM
// ZOSLIB
// (C) Copyright IBM Corp. 2022. All Rights Reserved.
// US Government Users Restricted Rights - Use, duplication
// or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
///////////////////////////////////////////////////////////////////////////////

#ifndef ZOS_SYS_SIGNALFD_H
#define ZOS_SYS_SIGNALFD_H

#include <stdint.h>
#include <signal.h>

/* Flags for signalfd.  */
enum {
  SFD_CLOEXEC = 02000000,
#define SFD_CLOEXEC SFD_CLOEXEC
  SFD_NONBLOCK = 00004000
#define SFD_NONBLOCK SFD_NONBLOCK
};


struct signalfd_siginfo {
  uint32_t ssi_signo;
  int32_t ssi_errno;
  int32_t ssi_code;
  uint32_t ssi_pid;
  uint32_t ssi_uid;
  int32_t ssi_fd;
  uint32_t ssi_tid;
  uint32_t ssi_band;
  uint32_t ssi_overrun;
  uint32_t ssi_trapno;
  int32_t ssi_status;
  uint32_t ssi_int;
  uint64_t ssi_ptr;
  uint64_t ssi_utime;
  uint64_t ssi_stime;
  uint64_t ssi_addr;
  /*
   * Pad strcture to 128 bytes. Remember to update the
   * pad size when you add new members. We use a fixed
   * size structure to avoid compatibility problems with
   * future versions, and we leave extra space for additional
   * members. We use fixed size members because this strcture
   * comes out of a read(2) and we really don't want to have
   * a compat on read(2).
   */
  uint8_t __pad[48];
};

#ifdef __cplusplus
extern "C" {
#endif

__Z_EXPORT extern int signalfd(int fd, const sigset_t *mask, int flags);

__Z_EXPORT extern int signalfd_close(int fd);

#ifdef __cplusplus
}
#endif

#endif
