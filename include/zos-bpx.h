///////////////////////////////////////////////////////////////////////////////
// Licensed Materials - Property of IBM
// ZOSLIB
// (C) Copyright IBM Corp. 2020. All Rights Reserved.
// US Government Users Restricted Rights - Use, duplication
// or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
///////////////////////////////////////////////////////////////////////////////

// APIs that implement some of the BPX* callable services for compilers
// that don't currently support OS linkage (as Woz); for details, see:
// https://www.ibm.com/docs/en/zos/2.4.0?topic=reference-callable-services-descriptions

#ifndef ZOS_BPX_H_
#define ZOS_BPX_H_

#ifndef _POSIX_SOURCE
#define _POSIX_SOURCE
#endif
#include <unistd.h>

typedef struct __bpxyatt {
  char att_id[4];    /* Eye-catcher="ATT " */
  short att_version; /* Version of this structure=3 */
  char att_res01[2]; /* (reserved) */

  /* ATTSETFLAGS1 = 1 byte */
  int att_modechg : 1,  /* X'80' 1=Change to mode indicated */
      att_ownerchg : 1, /* X'40' 1=Change to Owner indicated */
      att_setgen : 1,   /* X'20' 1=Set General Attributes */
      att_trunc : 1,    /* X'10' 1=Truncate Size */
      att_atimechg : 1, /* X'08' 1=Change the Atime */
      att_atimetod : 1, /* X'04' 1=Change Atime to Cur. Time */
      att_mtimechg : 1, /* X'02' 1=Change the Mtime */
      att_mtimetod : 1; /* X'01' 1=Change Mtime to Cur. Time */

  /* ATTSETFLAGS2 = 1 byte */
  int att_maaudit : 1,    /* X'80' 1=Modify auditor audit info */
      att_muaudit : 1,    /* X'40' 1=Modify user audit info */
      att_ctimechg : 1,   /* X'20' 1=Change the Ctime */
      att_ctimetod : 1,   /* X'10' 1=Change Ctime to Cur. Time */
      att_reftimechg : 1, /* X'08' 1=Change the RefTime */
      att_reftimetod : 1, /* X'04' 1=Change RefTime to Cur.Time */
      att_filefmtchg : 1, /* X'02' 1=Change File Format */
      att_res04 : 1;      /* X'01' (reserved flag bits) */

  /* ATTSETFLAGS3 = 1 byte */
  int att_res05 : 1,        /* X'80' (reserved flag bits) */
      att_charsetidchg : 1, /* X'40' 1=Change File Tag */
      att_lp64times : 1,    /* X'20' 1=Use 64-bit time values */
      att_seclabelchg : 1;  /* X'10' 1=Change Seclabel */

  char att_setflags4; /* Reserved */

  int att_mode; /* File Mode */
  int att_uid;  /* User ID of the owner of the file  */
  int att_gid;  /* Group ID of the Group of the file */

  /* 3 bytes */
  int att_opaquemask : 24; /* (reserved for ADSTAR use) */

  /* ATTVISIBLEMASK = 1 byte */
  int att_visblmaskres : 2,   /* (reserved for visible mask use) */
      att_nodelfilesmask : 1, /* X'20' 1=Files should not be deleted */
      att_sharelibmask : 1,   /* X'10' 1=Shared Library Mask */
      att_noshareasmask : 1,  /* X'08' 1=No Shareas Flag Mask */
      att_apfauthmask : 1,    /* X'04' 1=APF Authorized Flag Mask */
      att_progctlmask : 1,    /* X'02' 1=Prog. Control Flag Mask */
      att_visblmskrmain : 1;  /* (reserved flag mask bit) */

  /* ATTGENVALUE = 0 bytes */
  /* 3 bytes */
  int att_opaque : 24; /* (reserved for ADSTAR use) */

  /** ATTVISIBLE = 1 byte **/
  int att_visibleres : 2, /* (reserved for visible flag use) */
      att_nodelfiles : 1, /* X'20' 1=Files should not be deleted */
      att_sharelib : 1,   /* X'10' 1=Shared Library Flag */
      att_noshareas : 1,  /* X'08' 1=No Shareas Flag */
      att_apfauth : 1,    /* X'04' 1=APF Authorized Flag */
      att_progctl : 1,    /* X'02' 1=Program Controlled Flag */
      att_visblrmain : 1; /* (reserved flag mask bit) */

  int att_size_h; /* first word of size */
  int att_size_l; /* second word of size */
  int att_atime;  /* Time of last access */
  int att_mtime;  /* Time of last data modification */

  int att_auditoraudit; /* Area for auditor audit info */
  int att_useraudit;    /* Area for user audit info */

  int att_ctime;   /* Time of last file statuse change */
  int att_reftime; /* Reference Time */

  /* End of version 1 */

  char att_filefmt;  /* File Format */
  char att_res02[3]; /* (reserved for expansion) */
  int att_filetag;   /* File Tag */
  char att_res03[8]; /* (reserved for expansion) */

  /* End of version 2 */

  long att_atime64;      /* Time of last access */
  long att_mtime64;      /* Time of last data modification */
  long att_ctime64;      /* Time of last file statuse change */
  long att_reftime64;    /* Reference Time */
  char att_seclabel[8];  /* Security Label */
  char att_ver3res02[8]; /* (reserved for expansion) */

  /* End of version 3 */
} __bpxyatt_t;

#ifdef __cplusplus
extern "C" {
#endif

/* TODO(gabylb): zos - document */
char *__ptr32 *__ptr32 __uss_base_address(void);
void __bpx4kil(int pid, int signal, void *signal_options,
              int *return_value, int *return_code, int *reason_code);
void __bpx4frk(int *pid, int *return_code, int *reason_code);
void __bpx4ctw(unsigned int *secs, unsigned int *nsecs,
               unsigned int *event_list, unsigned int *secs_rem,
               unsigned int *nsecs_rem, int *return_value,
               int *return_code, int *reason_code);
void __bpx4gth(int *input_length, void **input_address,
               int *output_length, void **output_address,
               int *return_value, int *return_code, int *reason_code);
void __bpx4lcr(int pathname_length, char *pathname,
               int attributes_length, __bpxyatt_t *attributes,
               int *return_value, int *return_code, int *reason_code);

#ifdef __cplusplus
}
#endif
#endif  // ZOS_BPX_H_
