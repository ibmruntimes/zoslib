///////////////////////////////////////////////////////////////////////////////
//// Licensed Materials - Property of IBM
//// ZOSLIB
//// (C) Copyright IBM Corp. 2022. All Rights Reserved.
//// US Government Users Restricted Rights - Use, duplication
//// or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
/////////////////////////////////////////////////////////////////////////////////
//
#ifndef ZOS_LIMITS_H
#define ZOS_LIMITS_H

#include_next <limits.h>

#ifndef PATH_MAX
#define PATH_MAX _XOPEN_PATH_MAX
#endif

#ifndef NAME_MAX
#define NAME_MAX 255
#endif

/* z/OS system headers define TTY_NAME_MAX as 9 (POSIX minimum), which is
 * insufficient for actual PTY device paths like "/dev/pts/123".
 * Redefine to a safe value for applications using this constant. */

#ifdef TTY_NAME_MAX
#undef TTY_NAME_MAX
#endif
#define TTY_NAME_MAX 128

#endif
