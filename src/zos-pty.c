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
#include <pty.h>

int openpty(int *master, int *slave, char *name, const struct termios *termp, const struct winsize *winp) {
  int fd;
  char master_dev[20], slave_dev[20];

  *master = -1;
  *slave = -1;

  // Iterating through the 4-digit ptyp and ttyp devices
  int i=0;
  for (; i < 10000; i++) {  // Maximum 9999 for 4-digit format
    snprintf(master_dev, sizeof(master_dev), "/dev/ptyp%04d", i);
    snprintf(slave_dev, sizeof(slave_dev), "/dev/ttyp%04d", i);

    if ((*master = open(master_dev, O_RDWR | O_NOCTTY)) != -1) {
      if (grantpt(*master) != 0)
              goto error;

      if (unlockpt(*master) != 0)
              goto error;

      if ((*slave = open(slave_dev, O_RDWR | O_NOCTTY)) != -1) {
              break;  // Found an available pair
      }

      close(*master);
      *master = -1;
    }
  }

  if(i == 10000)
      goto error;

  if (termp && tcsetattr(*slave, TCSAFLUSH, termp) == -1) {
    perror("tcsetattr() error");
    goto error;
  }
  if (winp && ioctl(*slave, TIOCSWINSZ, winp) == -1) {
    goto error;
  }

  // ignoring name, not passed and size is unknown in the API

  return 0;

error:
  if (*slave != -1) {
    close(*slave);
    *slave = -1;
  }
  if (*master != -1) {
    close(*master);
    *master = -1;
  }
  return -1;
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

pid_t forkpty(int *amaster, char *name, const struct termios *termp, const struct winsize *winp) {
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
