///////////////////////////////////////////////////////////////////////////////
//// Licensed Materials - Property of IBM
//// ZOSLIB
//// (C) Copyright IBM Corp. 2022. All Rights Reserved.
//// US Government Users Restricted Rights - Use, duplication
//// or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
/////////////////////////////////////////////////////////////////////////////////
//
#ifndef ZOS_TERMIOS_H
#define ZOS_TERMIOS_H

#include "zos-macros.h"

#include_next <termios.h>

#ifdef __cplusplus
extern "C" {
#endif

__Z_EXPORT void cfmakeraw(struct termios *termios_p);

#ifdef __cplusplus
}
#endif

#endif
