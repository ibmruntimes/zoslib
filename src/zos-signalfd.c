/////////////////////////////////////////////////////////////////////////////////
//// Licensed Materials - Property of IBM
//// ZOSLIB
//// (C) Copyright IBM Corp. 2022. All Rights Reserved.
//// US Government Users Restricted Rights - Use, duplication
//// or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
/////////////////////////////////////////////////////////////////////////////////

#define _AE_BIMODAL 1

#include "zos-base.h"
#include <stdlib.h>
#include <fcntl.h>
#include <sys/signalfd.h>
#include <string.h>

static int signal_pipe[2] = {-1, -1};

void sigHandler(int signo, siginfo_t *info, void *ucontext) {
  struct signalfd_siginfo siginfo;

  memset(&siginfo, 0, sizeof(struct signalfd_siginfo));
  siginfo.ssi_signo = signo;
  siginfo.ssi_code = info->si_code;
  siginfo.ssi_pid = info->si_pid;
  siginfo.ssi_uid = info->si_uid;
  siginfo.ssi_errno = info->si_errno;
  siginfo.ssi_status = info->si_status;
  siginfo.ssi_band = info->si_band;


  // Write the signal info to the pipe's write end
  if (write(signal_pipe[1], &siginfo, sizeof(struct signalfd_siginfo)) == -1) {
      perror("write");
  }
}

int signalfd(int fd, const sigset_t *mask, int flags)
{
  if (pipe(signal_pipe) == -1) {
    perror("pipe");
    return -1;
  }

  if(flags & SFD_CLOEXEC) {
    // Set the FD_CLOEXEC flag on both ends of the pipe
    if (fcntl(signal_pipe[0], F_SETFD, FD_CLOEXEC) == -1) {
      perror("fcntl(pipefd[0], FD_CLOEXEC)");
      goto error;
    }
    if (fcntl(signal_pipe[1], F_SETFD, FD_CLOEXEC) == -1) {
      perror("fcntl(pipefd[1], FD_CLOEXEC)");
      goto error;
    }
  }

  sigprocmask(SIG_UNBLOCK, mask, NULL);

  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_flags = SA_SIGINFO;   // Enable additional signal information
  sa.sa_sigaction = sigHandler;  // Set our custom handler

  for(int signo = 1; signo < 36; signo++) {
    if(sigismember(mask, signo)) {
      if(sigaction(signo, &sa, NULL) == -1) {
        perror("sigaction");
      }
    }
  }
  
  return signal_pipe[0];

error:
  signalfd_close(signal_pipe[0]);
  return -1;

}

int signalfd_close(int fd)
{
  (void)fd;
  if (signal_pipe[0] != -1) close(signal_pipe[0]);
  if (signal_pipe[1] != -1) close(signal_pipe[1]);
  signal_pipe[0] = signal_pipe[1] = -1;
  return 0;
}
