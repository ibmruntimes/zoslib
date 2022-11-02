///////////////////////////////////////////////////////////////////////////////
////// Licensed Materials - Property of IBM
////// ZOSLIB
////// (C) Copyright IBM Corp. 2022. All Rights Reserved.
////// US Government Users Restricted Rights - Use, duplication
////// or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
///////////////////////////////////////////////////////////////////////////////////

#ifndef _ZOS_SPAWN_H
#define _ZOS_SPAWN_H

#include_next <spawn.h>

#ifdef __cplusplus
extern "C" {
#endif

#define POSIX_SPAWN_SETPGROUP 0x02
#define POSIX_SPAWN_SETSIGMASK 0x08
#define POSIX_SPAWN_USEVFORK 0x40
#define POSIX_SPAWN_SETSIGDEF 0x01
#define POSIX_SPAWN_SETSCHEDPARAM 0x04
#define POSIX_SPAWN_SETSCHEDULER 0x10
#define POSIX_SPAWN_RESETIDS 0x20

typedef struct posix_spawn_file_actions_t {
  struct _spawn_actions *actions;
} posix_spawn_file_actions_t;

typedef struct posix_spawnattr_t {
  sigset_t *mask;
  short flags;
} posix_spawnattr_t;

int posix_spawn_file_actions_init(posix_spawn_file_actions_t *);
int posix_spawn_file_actions_addclose(posix_spawn_file_actions_t *,
                                      int pipe_fd);
int posix_spawn_file_actions_addopen(posix_spawn_file_actions_t *, int,
                                     const char *, int flags, mode_t);
int posix_spawn_file_actions_adddup2(posix_spawn_file_actions_t *, int pipe_fd,
                                     int fd);
int posix_spawn_file_actions_destroy(posix_spawn_file_actions_t *);

int posix_spawnattr_init(posix_spawnattr_t *);
int posix_spawnattr_setsigmask(posix_spawnattr_t *, sigset_t *mask);
int posix_spawnattr_setflags(posix_spawnattr_t *, short flags);
int posix_spawnattr_destroy(posix_spawnattr_t *);

int posix_spawn(pid_t *pid, const char *cmd,
                const posix_spawn_file_actions_t *act,
                const posix_spawnattr_t *, char *const args[],
                char *const env[]);

#ifdef __cplusplus
}
#endif

#endif
