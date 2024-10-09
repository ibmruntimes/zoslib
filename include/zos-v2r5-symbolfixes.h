///////////////////////////////////////////////////////////////////////////////
// ZOSLIB
// (C) Copyright IBM Corp. 2023. All Rights Reserved.
// US Government Users Restricted Rights - Use, duplication
// or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
///////////////////////////////////////////////////////////////////////////////

#ifndef ZOS_V2R5_SYMBOLFIXES_H
#define ZOS_V2R5_SYMBOLFIXES_H

// This enables builds on >=V2R5 systems when the target is <V2R5
// Redefines >=V2R5 symbols with _undefined suffix to trigger a linker 
// error so they are not detected by configure scripts
#if (__TARGET_LIB__ < 0x42050000)
#pragma redefine_extname readlinkat readlinkat_undefined
#pragma redefine_extname openat openat_undefined
#pragma redefine_extname linkat linkat_undefined
#pragma redefine_extname faccessat faccessat_undefined
#pragma redefine_extname fstatat fstatat_undefined
#pragma redefine_extname unlinkat unlinkat_undefined
#pragma redefine_extname symlinkat symlinkat_undefined
#pragma redefine_extname renameat rename_undefined
#pragma redefine_extname getrandom getrandom_undefined
#pragma redefine_extname pipe2 pipe2_undefined
#pragma redefine_extname fsstatfs fsstatfs_undefined
#pragma redefine_extname dprintf dprintf_undefined
#pragma redefine_extname dirfd dirfd_undefined
#pragma redefine_extname fchmodat fchmodat_undefined
#pragma redefine_extname mkdirat mkdirat_undefined
#pragma redefine_extname mkfifoat mkfifoat_undefined
#pragma redefine_extname mknodat mknodat_undefined
#pragma redefine_extname renameat2 renameat2_undefined
#pragma redefine_extname futimesat futimesat_undefined
#pragma redefine_extname fchownat fchownat_undefined
#pragma redefine_extname strchrnul strchrnul_undefined
#pragma redefine_extname sethostname sethostname_undefined
#pragma redefine_extname syncfs syncfs_undefined
#pragma redefine_extname sysinfo sysinfo_undefined
#pragma redefine_extname fdatasync fdatasync_undefined
#pragma redefine_extname inotify_init inotify_init_undefined
#pragma redefine_extname prctl prctl_undefined
#pragma redefine_extname fstatfs fstatfs_undefined
#pragma redefine_extname setresuid setresuid_undefined
#pragma redefine_extname setresgid setresgid_undefined
#pragma redefine_extname dup3 dup3_undefined
#pragma redefine_extname shm_open shm_open_undefined
#pragma redefine_extname utimensat utimensat_undefined
#pragma redefine_extname fdopendir fdopendir_undefined
#pragma redefine_extname pthread_condattr_setclock pthread_condattr_setclock_undefined
#endif

#endif // ZOS_V2R5_SYMBOLFIXES_H
