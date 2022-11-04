///////////////////////////////////////////////////////////////////////////////
//// Licensed Materials - Property of IBM
//// ZOSLIB
//// (C) Copyright IBM Corp. 2022. All Rights Reserved.
//// US Government Users Restricted Rights - Use, duplication
//// or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
/////////////////////////////////////////////////////////////////////////////////

#define _POSIX_SOURCE
#define _OPEN_SYS
#include <sys/types.h>
#include <mntent.h>
#include <sys/mount.h>
#include <sys/mntent.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

static const char* fstype[] = { "UNK", "MVS", "NFS", "PIPE", "SOCKET" };
#define NUM_FSTYPE 5

int getmntinfo(struct statfs **statfsp, int flags)
{
  struct mntlst {
    MNTE3H hdr;
    W_MNTENT3 tbl[0];
  };
  struct mntlst* mntlst;
  struct statfs* statfs;

  int entries = w_getmntent(NULL, 0);
  int i;

  if (entries == 0) {
    return 0; /* no mounted file systems */
  }

  if (entries < 0) {
    errno = EIO;
    return 0; /* I/O error */
  }

  if (flags != MNT_NOWAIT) { /* no support for MNT_WAIT */
    errno = EIO;
    return 0;
  }

  mntlst = malloc(sizeof(MNTE3H) + sizeof(W_MNTENT3)*(entries));
  if (!mntlst) {
    return 0; /* OOM */
  }
  *statfsp = malloc(sizeof(struct statfs)*entries);
  if (!statfsp) {
    free(mntlst);
    return 0; /* OOM */
  }

  memset(&mntlst->hdr, 0x00, sizeof(MNTE3H));
  memcpy(&mntlst->hdr.mnt3H_cbid, MNTE3H_ID, sizeof(MNTE3H_ID)-1);
  mntlst->hdr.mnt3H_cblen = sizeof(MNTE3H);
  mntlst->hdr.mnt3H_bodylen = sizeof(W_MNTENT3);
  entries = w_getmntent((char*)mntlst, sizeof(MNTE3H) + sizeof(W_MNTENT3)*entries);

  statfs = *statfsp;

  for (i=0; i<entries; ++i) {
    const char* from = mntlst->tbl[i].mnt3_fsname;
    const char* on = mntlst->tbl[i].mnt3_mountpoint;
    size_t fromlen = strlen(from);
    size_t onlen = strlen(on);
    int type = mntlst->tbl[i].mnt3_fstype;

    memset(&statfs[i], 0x00, sizeof(struct statfs));

    if (fromlen > MNAMELEN) {
      fromlen = MNAMELEN-1;
    }
    memcpy(statfs[i].f_mntfromname, from, fromlen+1);

    if (onlen > MNAMELEN) {
      onlen = MNAMELEN-1;
    }
    memcpy(statfs[i].f_mntonname, on, onlen+1);

    strcpy(statfs[i].f_fstypename, (type > NUM_FSTYPE) ? fstype[0] : fstype[type]);
  }
  free(mntlst);

  return entries; 
}


