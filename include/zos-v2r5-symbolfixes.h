///////////////////////////////////////////////////////////////////////////////
// ZOSLIB
// (C) Copyright IBM Corp. 2023. All Rights Reserved.
// US Government Users Restricted Rights - Use, duplication
// or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
///////////////////////////////////////////////////////////////////////////////

#ifndef ZOS_V2R5_SYMBOLFIXES_H
#define ZOS_V2R5_SYMBOLFIXES_H

// Redefines V2R5+ symbols with _undefined suffix to trigger a linker 
// error so they are not detected by autotools-based configure scripts. 
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
#pragma redefine_extname asprintf asprintf_undefined
#pragma redefine_extname eventfd eventfd_undefined
#pragma redefine_extname wait4 wait4_undefined
#pragma redefine_extname madvise madvise_undefined

// Additional v2r5 functions based on analysis in https://github.com/orgs/zopencommunity/discussions/855#discussioncomment-12192678
#pragma redefine_extname accept4 accept4_undefined
#pragma redefine_extname clone clone_undefined
#pragma redefine_extname epoll_create epoll_create_undefined
#pragma redefine_extname epoll_create1 epoll_create1_undefined
#pragma redefine_extname epoll_ctl epoll_ctl_undefined
#pragma redefine_extname epoll_pwait epoll_pwait_undefined
#pragma redefine_extname epoll_wait epoll_wait_undefined
#pragma redefine_extname fdclosedir fdclosedir_undefined
//#pragma redefine_extname flock flock_undefined // Defined in zoslib
#pragma redefine_extname fremovexattr fremovexattr_undefined
#pragma redefine_extname fsetxattr fsetxattr_undefined
//#pragma redefine_extname getentropy getentropy_undefined // Defined in zoslib
#pragma redefine_extname getopt_long getopt_long_undefined
#pragma redefine_extname getpwent_r getpwent_r_undefined
#pragma redefine_extname inet_aton inet_aton_undefined
#pragma redefine_extname inotify_add_watch inotify_add_watch_undefined
#pragma redefine_extname inotify_init1 inotify_init1_undefined
#pragma redefine_extname inotify_rm_watch inotify_rm_watch_undefined
#pragma redefine_extname lremovexattr lremovexattr_undefined
#pragma redefine_extname lsetxattr lsetxattr_undefined
#pragma redefine_extname pivot_root pivot_root_undefined
#pragma redefine_extname removexattr removexattr_undefined
#pragma redefine_extname setns setns_undefined
#pragma redefine_extname setxattr setxattr_undefined
#pragma redefine_extname statfs statfs_undefined
#pragma redefine_extname umount2 umount2_undefined
#pragma redefine_extname unshare unshare_undefined
//#pragma redefine_extname vasprintf vasprintf_undefined // Defined in zoslib
#pragma redefine_extname openat2 openat2_undefined
#pragma redefine_extname syscall syscall_undefined
#pragma redefine_extname fgetxattr fgetxattr_undefined
#pragma redefine_extname flistxattr flistxattr_undefined
#pragma redefine_extname getxattr getxattr_undefined
#pragma redefine_extname lgetxattr lgetxattr_undefined
#pragma redefine_extname listxattr listxattr_undefined
#pragma redefine_extname llistxattr llistxattr_undefined
#endif

#endif // ZOS_V2R5_SYMBOLFIXES_H
