///////////////////////////////////////////////////////////////////////////////
// Licensed Materials - Property of IBM
// ZOSLIB
// (C) Copyright IBM Corp. 2022. All Rights Reserved.
// US Government Users Restricted Rights - Use, duplication
// or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
///////////////////////////////////////////////////////////////////////////////

#ifndef ZOS_SYS_MOUNT_H_
#define ZOS_SYS_MOUNT_H_

#include "zos-macros.h"

#include <sys/types.h>

#define MFSNAMELEN     15 /* length of fs type name, not inc. nul */
#define MNAMELEN	     90 /* length of buffer for returned name */

#define MNT_WAIT        1
#define MNT_NOWAIT      2 /* only NOWAIT supported with getmntinfo */

#define	MNT_RDONLY	0x00000001

#if defined(__cplusplus)
extern "C" {
#endif

struct statfs { 
#if 0 /* not implemented yet */
  short	 f_otype;    /* type of file system (reserved: zero) */
  short	 f_oflags;   /* copy of mount flags (reserved: zero) */
  long	 f_bsize;    /* fundamental file system block size */
  long	 f_iosize;   /* optimal transfer block size */
  long	 f_blocks;   /* total data blocks in file system */
  long	 f_bfree;    /* free blocks in fs */
  long	 f_bavail;   /* free blocks avail to non-superuser */
  long	 f_files;    /* total file nodes in file system */
  long	 f_ffree;    /* free file nodes in fs */
  fsid_t  f_fsid;     /* file system id */
  uid_t	 f_owner;    /* user that mounted the file system */
  short	 f_reserved1;	     /* reserved for future use */
  short	 f_type;     /* type of file system (reserved) */
  long	 f_reserved2[2];     /* reserved for future use */
#else
  long   f_reserved[15];           /* reserved as 0 */
#endif
  long   f_flags;    /* copy of mount flags (reserved) */
  char	 f_fstypename[MFSNAMELEN]; /* fs type name */
  char	 f_mntonname[MNAMELEN];    /* directory on which mounted */
  char	 f_mntfromname[MNAMELEN];  /* mounted file system */
};

__Z_EXPORT int getmntinfo(struct statfs **mntbufp, int flags);

#if defined(__cplusplus)
}
#endif

#endif
