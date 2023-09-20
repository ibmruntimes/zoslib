///////////////////////////////////////////////////////////////////////////////
// Licensed Materials - Property of IBM
// ZOSLIB
// (C) Copyright IBM Corp. 2022. All Rights Reserved.
// US Government Users Restricted Rights - Use, duplication
// or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
///////////////////////////////////////////////////////////////////////////////

#ifndef ZOS_V2R5_FIXUPS_H
#define ZOS_V2R5_FIXUPS_H

// This enables builds on >=V2R5 systems when the target is <V2R5
// Redefines >=V2R5 symbols with _undefined suffix to trigger a linker 
// error so they are not detected by configure scripts
#if (__EDC_TARGET < 0x42050000)
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
#pragma redefine_extname getline getline_undefined
#pragma redefine_extname dprintf dprintf_undefined
#pragma redefine_extname dirfd dirfd_undefine
#endif

#endif // ZOS_V2R5_FIXUPS_H