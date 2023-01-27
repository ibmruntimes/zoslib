///////////////////////////////////////////////////////////////////////////////
//// Licensed Materials - Property of IBM
//// ZOSLIB
//// (C) Copyright IBM Corp. 2022. All Rights Reserved.
//// US Government Users Restricted Rights - Use, duplication
//// or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
/////////////////////////////////////////////////////////////////////////////////

#include <spawn.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#define _POSIX_SOURCE
#include <unistd.h>

enum ActionKinds { op_open, op_close, op_dup2};
struct _spawn_actions {
  ActionKinds op;
  int fd;
  struct Open_info {
    const char *path;
    int oflags;
    mode_t mode;
  } open_info;
  int new_fd;
  _spawn_actions * next;
};


// -----------  Actions

int posix_spawn_file_actions_init(posix_spawn_file_actions_t* act) {
  if (act==nullptr)
    return EINVAL;
  act->actions = nullptr;
  return 0;
}

void printActions(const posix_spawn_file_actions_t* act) {
  fprintf(stderr, "posix_spawn_file_actions_t:%p:", act);
  if (!act) {
    fprintf(stderr, "\n");
    return;
  }
  for (const _spawn_actions *p = act->actions; p ; p = p->next) {
    switch (p->op) {
    case op_open:
      fprintf(stderr, "open-%p(%d,%s,%08x,%08x):", p, p->fd, p->open_info.path, p->open_info.oflags, p->open_info.mode);
      break;
    case op_close:
      fprintf(stderr, "close-%p(%d):", p, p->fd);
      break;
    case op_dup2:
      fprintf(stderr, "dup2-%p(%d,%d):", p, p->fd, p->new_fd);
      break;
    default:
      fprintf(stderr, "unkown-%p(%d):", p, p->op);
      break;
    }
  }
  fprintf(stderr, "\n");
}

static _spawn_actions* appendAction(posix_spawn_file_actions_t* act) {
  _spawn_actions * tail = act->actions;
  _spawn_actions * p = (_spawn_actions*) malloc(sizeof(_spawn_actions));
  if (p==nullptr) {
    return nullptr;
  }
  p->next = nullptr;
  if (tail==nullptr) {
    act->actions = p;
  } else {
    for (; tail->next; tail=tail->next) {
    }
    tail->next = p;
  }
  return p;
}

int posix_spawn_file_actions_addclose(posix_spawn_file_actions_t* act, int pipe_fd) {
  if (act==nullptr)
    return EINVAL;
  if (pipe_fd<0 || pipe_fd>sysconf(_SC_OPEN_MAX))
    return EBADF;

  _spawn_actions * p = appendAction(act);
  if (!p)
    return ENOMEM;
  p->op = op_close;
  p->fd = pipe_fd;

  return 0;
  
}

int posix_spawn_file_actions_addopen(posix_spawn_file_actions_t* act, int pipe_fd, const char *path, int flags, mode_t mode) {
  if (act==nullptr)
    return EINVAL;
  if (pipe_fd<0 || pipe_fd>sysconf(_SC_OPEN_MAX))
    return EBADF;

  _spawn_actions * p = appendAction(act);
  if (!p)
    return ENOMEM;
  p->op = op_open;
  p->fd = pipe_fd;
  p->open_info.path = path;  
  p->open_info.oflags = flags;  
  p->open_info.mode = mode;  
  return 0;
}

int posix_spawn_file_actions_adddup2(posix_spawn_file_actions_t* act, int pipe_fd, int nfd) {
  if (act==nullptr)
    return EINVAL;
  if (pipe_fd<0 || pipe_fd>sysconf(_SC_OPEN_MAX))
    return EBADF;
  if (nfd<0 || nfd>sysconf(_SC_OPEN_MAX))
    return EBADF;
  _spawn_actions * p = appendAction(act);
  if (!p)
    return ENOMEM;
  p->op = op_dup2;
  p->fd = pipe_fd;
  p->new_fd = nfd;
  return 0;
}

int posix_spawn_file_actions_destroy(posix_spawn_file_actions_t* act) {
  if (act==nullptr)
    return EINVAL;

  _spawn_actions *cur = act->actions;
  for (; cur;) {
    _spawn_actions *next = cur->next;
    free(cur);
    cur=next;
  }
  return 0;
}

// --------------  Attributes
int posix_spawnattr_init(posix_spawnattr_t* attr){
  if (attr==nullptr)
    return EINVAL;
  attr->flags = 0;
  attr->mask = 0;
  return 0;
}

int posix_spawnattr_setsigmask(posix_spawnattr_t* attr, sigset_t *mask){
  if (attr==nullptr)
    return EINVAL;
  attr->mask = mask;
  return 0;
}


int posix_spawnattr_setflags(posix_spawnattr_t* attr, short flags) {
  if (attr==nullptr)
    return EINVAL;

  attr->flags = flags;
  return 0;
}

int posix_spawnattr_destroy(posix_spawnattr_t* attr) {
  if (attr==nullptr)
    return EINVAL;
  return 0;
}

int posix_spawn(pid_t *pid, const char *cmd, const posix_spawn_file_actions_t *act, const posix_spawnattr_t* attr, char * const args[], char * const env[]) {

  if (pid==nullptr)
    return EINVAL;
#if HAS_VFORK
  const short flags_with_actions = POSIX_SPAWN_SETSIGMASK|POSIX_SPAWN_SETSIGDEF|
          POSIX_SPAWN_SETSCHEDPARAM|POSIX_SPAWN_SETSCHEDULER|
          POSIX_SPAWN_SETPGROUP|POSIX_SPAWN_RESETIDS;
  if (attr && ((attr->flags & POSIX_SPAWN_USEVFORK) ||
      (act->actions==nullptr && !(attr->flags&flags_with_actions))))
  {
    // Simple fork() with no clean up actions
    *pid = vfork();
  } else
#endif
  {
    *pid = fork();
  }

  if (*pid < 0)
    return 1;

  if (*pid == 0) {
    // In the child
    int rc = 0;
    if (attr && attr->flags & POSIX_SPAWN_SETSIGMASK) {
      if ((rc=sigprocmask(SIG_SETMASK, attr->mask, 0)) < 0)
        return rc;      
    }

    if (attr && attr->flags & POSIX_SPAWN_SETPGROUP) {
      if ((rc=setpgid(0, 0)) < 0)
        return rc;
    }
    
    if (act) {
      _spawn_actions *cur = act->actions;
      for (; cur; cur=cur->next) {
        switch (cur->op) {
          case op_close: close(cur->fd); break;
          case op_open: {
            int fd = open(cur->open_info.path, cur->open_info.oflags, cur->open_info.mode);
            if (fd < 0)
              return fd;
            if (fd != cur->fd) {
              if ((rc=dup2(fd, cur->fd)) < 0)
                return rc;
              close(fd);
            }
            break;
          }
          case op_dup2:
            if ((rc=dup2(cur->fd, cur->new_fd)) < 0)
              return rc;
            break;
          default: return 127;
        }
      }
    }
  execve(cmd, args, env);
  return 127;
  }
  return 0;
}
