///////////////////////////////////////////////////////////////////////////////
// Licensed Materials - Property of IBM
// ZOSLIB
// (C) Copyright IBM Corp. 2021. All Rights Reserved.
// US Government Users Restricted Rights - Use, duplication
// or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
///////////////////////////////////////////////////////////////////////////////

#ifndef ZOS_UNISTD_H_
#define ZOS_UNISTD_H_

#include "zos-macros.h"
#include <sys/types.h>

#if defined(__cplusplus)
extern "C" {
#endif
__Z_EXPORT int __pipe_ascii(int [2]);
__Z_EXPORT int __close(int);
__Z_EXPORT int __sysconf(int name);
__Z_EXPORT ssize_t __write_ds_file(int fd, const void *buf, size_t count);
__Z_EXPORT ssize_t __read_ds_file(int fd, void *buf, size_t count);
__Z_EXPORT off_t __lseek_ds_file(int fd, off_t offset, int whence);

#if defined(__cplusplus)
}
#endif

#if defined(ZOSLIB_OVERRIDE_CLIB) || defined(ZOSLIB_OVERRIDE_CLIB_UNISTD)

#undef pipe 
#define pipe __pipe_replaced
#undef close
#define close __close_replaced
#undef sysconf
#define sysconf __sysconf_replaced
#undef readlink
#define readlink __readlink_replaced
#undef write
#define write __write_replaced
#undef read
#define read __read_replaced
#include_next <unistd.h>
#undef pipe
#undef close
#undef sysconf
#undef readlink
#undef write
#undef read

#if defined(__cplusplus)
extern "C" {
#endif
/**
 * Same as C pipe but tags pipes as ASCII (819)
 */
__Z_EXPORT int pipe(int [2]) __asm("__pipe_ascii");
__Z_EXPORT int close(int) __asm("__close");
__Z_EXPORT int close(int) __asm("__close");
__Z_EXPORT int sysconf(int name) __asm("__sysconf");
__Z_EXPORT ssize_t readlink(const char *path, char *buf, size_t bufsiz) __asm("__readlink");
__Z_EXPORT ssize_t write(int fd, const void *buf, size_t count) __asm("__write_ds_file");
__Z_EXPORT ssize_t read(int fd, void *buf, size_t count) __asm("__read_ds_file");
__Z_EXPORT off_t lseek(int fd, off_t offset, int whence) __asm("__lseek_ds_file");

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

#include <zos-getentropy.h>

#if defined(__cplusplus)
}
#endif

#ifndef _SC_NPROCESSORS_ONLN
#define _SC_NPROCESSORS_ONLN 58 /* match linux */
#endif

#ifndef _SC_NPROCESSORS_CONF
#define _SC_NPROCESSORS_CONF _SC_NPROCESSORS_ONLN /* TODO: implement */
#endif

#endif
