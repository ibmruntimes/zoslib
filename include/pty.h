///////////////////////////////////////////////////////////////////////////////
//// Licensed Materials - Property of IBM
//// ZOSLIB
//// (C) Copyright IBM Corp. 2022. All Rights Reserved.
//// US Government Users Restricted Rights - Use, duplication
//// or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
/////////////////////////////////////////////////////////////////////////////////
//
#ifndef ZOS_PTY_H
#define ZOS_PTY_H

#include "zos-macros.h"
#include <termios.h>

#ifdef __cplusplus
extern "C" {
#endif


/* Create pseudo tty master slave pair with NAME and set terminal
   attributes according to TERMP and WINP and return handles for both
   ends in AMASTER and ASLAVE.  */
__Z_EXPORT extern int openpty (int *__amaster, int *__aslave, char *__name,
                    const struct termios *__termp,
                    const struct winsize *__winp);

/* Create child process and establish the slave pseudo terminal as the
   child's controlling terminal.  */
__Z_EXPORT extern int forkpty (int *__amaster, char *__name,
                    const struct termios *__termp,
                    const struct winsize *__winp);


#ifdef __cplusplus
}
#endif

#endif
