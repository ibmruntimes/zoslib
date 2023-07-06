///////////////////////////////////////////////////////////////////////////////
// Licensed Materials - Property of IBM
// ZOSLIB
// (C) Copyright IBM Corp. 2021. All Rights Reserved.
// US Government Users Restricted Rights - Use, duplication
// or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
///////////////////////////////////////////////////////////////////////////////

#ifndef ZOS_SYSMACROS_H_
#define ZOS_SYSMACROS_H_

#define major(x)    (((unsigned)(x) >> 8) & 0x7f)
#define minor(x)    ((x) & 0xff)
#define makedev(x, y) (unsigned short)(((x) << 8) | ((y) & 0xff))

#endif
