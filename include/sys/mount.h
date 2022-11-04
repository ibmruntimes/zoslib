#ifndef __ZOSLIB_MOUNT__
  #define __ZOSLIB_MOUNT__ 1

  typedef struct { int32_t val[2]; } fsid_t;

  #define MFSNAMELEN      15 /* length of fs type name, not inc. nul */
  #define MNAMELEN	     90 /* length of buffer for returned name */

  struct statfs { /* when _DARWIN_FEATURE_64_BIT_INODE is NOT defined */
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
	  long	 f_flags;    /* copy of mount flags (reserved) */
	  long	 f_reserved2[2];     /* reserved for future use */
	  char	 f_fstypename[MFSNAMELEN]; /* fs type name */
	  char	 f_mntonname[MNAMELEN];    /* directory on which mounted */
	  char	 f_mntfromname[MNAMELEN];  /* mounted file system */
	  char	 f_reserved3;	     /* reserved for future use */
	  long	 f_reserved4[4];     /* reserved for future use */
  };

#endif
