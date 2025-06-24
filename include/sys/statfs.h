///////////////////////////////////////////////////////////////////////////////
// Licensed Materials - Property of IBM
// ZOSLIB
// (C) Copyright IBM Corp. 2021. All Rights Reserved.
// US Government Users Restricted Rights - Use, duplication
// or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
///////////////////////////////////////////////////////////////////////////////

#ifndef ZOS_STATFS_H_
#define ZOS_STATFS_H_

#include "zos-macros.h"
#include <features.h>

// struct statfs in zoslib's sys/mount.h should be used and not the one in LE's sys/statfs.h
#if defined(ZOSLIB_OVERRIDE_CLIB) || defined(ZOSLIB_OVERRIDE_CLIB_STATFS)
/* Modify function names in header to avoid conflict with new prototypes */
#undef statfs
#define statfs __statfs_replaced
#endif

#include_next <sys/statfs.h>

#if defined(ZOSLIB_OVERRIDE_CLIB) || defined(ZOSLIB_OVERRIDE_CLIB_STATFS)

#undef statfs 

#endif

#endif
