
///////////////////////////////////////////////////////////////////////////////
// Licensed Materials - Property of IBM
// ZOSLIB
// (C) Copyright IBM Corp. 2021. All Rights Reserved.
// US Government Users Restricted Rights - Use, duplication
// or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
///////////////////////////////////////////////////////////////////////////////

#ifndef ZOS_DIRENT_H_
#define ZOS_DIRENT_H_

#include "zos-macros.h"
#include <features.h>


#if defined(ZOSLIB_OVERRIDE_CLIB) || defined(ZOSLIB_OVERRIDE_CLIB_DIRENT)
/* Modify function names in header to avoid conflict with new prototypes 
TODO: remove this when LE's fdopendir is fixed */

#undef fdopendir
#define fdopendir __fdopendir_replaced
#endif
#include_next <dirent.h>

#if defined(ZOSLIB_OVERRIDE_CLIB) || defined(ZOSLIB_OVERRIDE_CLIB_DIRENT)

#undef fdopendir 

#endif

#endif

