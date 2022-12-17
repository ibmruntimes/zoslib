///////////////////////////////////////////////////////////////////////////////
// Licensed Materials - Property of IBM
// ZOSLIB
// (C) Copyright IBM Corp. 2021. All Rights Reserved.
// US Government Users Restricted Rights - Use, duplication
// or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
///////////////////////////////////////////////////////////////////////////////

#ifndef ZOS_UNISTD_H_
#define ZOS_UNISTD_H_

#define __XPLAT 1
#include "zos-macros.h"

#if defined(__cplusplus)
extern "C" {
#endif
__Z_EXPORT int __pipe_ascii(int [2]);
__Z_EXPORT int __close(int);
#if defined(__cplusplus)
}
#endif

#if defined(ZOSLIB_OVERRIDE_CLIB) || defined(ZOSLIB_OVERRIDE_CLIB_UNISTD)

#undef pipe 
#define pipe __pipe_replaced
#undef close
#define close __close_replaced
#include_next <unistd.h>
#undef pipe
#undef close

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * Same as C pipe but tags pipes as ASCII (819)
 */
__Z_EXPORT int pipe(int [2]) asm("__pipe_ascii");
__Z_EXPORT int close(int) asm("__close");

#if defined(__cplusplus)
}
#endif
#else
#include_next <unistd.h>
#endif

#if defined(__cplusplus)
extern "C" {
#endif

#if (__EDC_TARGET < 0x42050000) && defined(ZOSLIB_ENABLE_V2R5_FEATURES)
__Z_EXPORT extern int (*pipe2)(int pipefd[2], int flags);
__Z_EXPORT extern int (*getentropy)(void *, size_t);
#else

/**
 * Execute a file.
 * \param [in] name used to construct a pathname that identifies the new
 *  process image file.
 * \param [in] argv an array of character pointers to NULL-terminated strings.
 * \param [in] envp an array of character pointers to NULL-terminated strings.
 * \return if successful, it doesn't return; otherwise, it returns -1 and sets
 *  errno.
 */
__Z_EXPORT int execvpe(const char *name, char *const argv[],
                       char *const envp[]);
#endif

#if defined(__cplusplus)
}
#endif

#endif
