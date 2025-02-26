/////////////////////////////////////////////////////////////////////////////////
//// Licensed Materials - Property of IBM
//// ZOSLIB
//// (C) Copyright IBM Corp. 2022. All Rights Reserved.
//// US Government Users Restricted Rights - Use, duplication
//// or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
/////////////////////////////////////////////////////////////////////////////////

#define _AE_BIMODAL 1

#include "zos-base.h"
#include <fcntl.h>
#include <sys/ioctl.h>
#include <termios.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <pty.h>

int openpty(int *master, int *slave, char *name, const struct termios *termp,
            const struct winsize *winp) {
  int mfd, sfd;
  char *slave_name;

  /* Open the master pty device */
  mfd = posix_openpt(O_RDWR | O_NOCTTY);
  if (mfd < 0) {
    perror("posix_openpt");
    return -1;
  }

  // Needed for z/OS so that the characters are not garbled if ptyp* is untagged
  struct f_cnvrt cvtreq = {SETCVTON, 0, 1047};
  fcntl(mfd, F_CONTROL_CVT, &cvtreq);

  if (grantpt(mfd) != 0) {
    perror("grantpt");
    close(mfd);
    return -1;
  }

  if (unlockpt(mfd) != 0) {
    perror("unlockpt");
    close(mfd);
    return -1;
  }

  slave_name = ptsname(mfd);
  if (slave_name == NULL) {
    perror("ptsname");
    close(mfd);
    return -1;
  }

  /* Note: Ensure that the caller provides a buffer large enough to hold the
   * name. */
  if (name) {
    strncpy(name, slave_name, 128);
    name[127] = '\0'; // Ensure null-termination
  }

  /* Open the slave pty device */
  sfd = open(slave_name, O_RDWR | O_NOCTTY);
  if (sfd < 0) {
    perror("open slave pty");
    close(mfd);
    return -1;
  }

  /* Set terminal attributes if provided */
  if (termp && tcsetattr(sfd, TCSAFLUSH, termp) == -1) {
    perror("tcsetattr");
    close(sfd);
    close(mfd);
    return -1;
  }

  /* Set window size if provided */
  if (winp && ioctl(sfd, TIOCSWINSZ, winp) == -1) {
    perror("ioctl TIOCSWINSZ");
    close(sfd);
    close(mfd);
    return -1;
  }

  *master = mfd;
  *slave = sfd;
  return 0;
}

static int login_tty(int fd) {
  setsid();

  dup2(fd, STDIN_FILENO);
  dup2(fd, STDOUT_FILENO);
  dup2(fd, STDERR_FILENO);
  if (fd > STDERR_FILENO) {
    close(fd);
  }

  return 0;
}

pid_t forkpty(int *amaster, char *name, const struct termios *termp,
              const struct winsize *winp) {
  int master, slave;
  if (openpty(&master, &slave, name, termp, winp) == -1) {
    return -1;
  }

  pid_t pid = fork();
  switch (pid) {
  case -1:
    close(master);
    close(slave);
    return -1;
  case 0:
    close(master);
    login_tty(slave);
    return 0;
  default:
    close(slave);
    *amaster = master;
    return pid;
  }
}
