///////////////////////////////////////////////////////////////////////////////
// Licensed Materials - Property of IBM
// ZOSLIB
// (C) Copyright IBM Corp. 2020. All Rights Reserved.
// US Government Users Restricted Rights - Use, duplication
// or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
///////////////////////////////////////////////////////////////////////////////

// APIs that implement various I/O operations.

#ifndef ZOS_IO_H_
#define ZOS_IO_H_

#include "zos-macros.h"

#include <stdarg.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#if TRACE_ON
/**
 * Prints information about a file descriptor.
 * \param [in] fd file descriptor.
 */
__Z_EXPORT void __fdinfo(int fd);

__Z_EXPORT void __perror(const char *str);
__Z_EXPORT int __dpoll(void *array, unsigned int count, int timeout);
__Z_EXPORT ssize_t __write(int fd, const void *buffer, size_t sz);
__Z_EXPORT ssize_t __read(int fd, void *buffer, size_t sz);
__Z_EXPORT int __close(int fd);
__Z_EXPORT int __open(const char *file, int oflag, int mode);
#endif // if TRACE_ON

/**
 * Debug Printf.
 * \return returns total number of bytes written to file descriptor
 */
__Z_EXPORT int dprintf(int fd, const char *, ...);

/**
 * Variadic Debug Printf.
 * \return returns total number of bytes written to file descriptor
 */
__Z_EXPORT int vdprintf(int fd, const char *, va_list ap);

/**
 * Dump to console.
 */
__Z_EXPORT void __dump(int fd, const void *addr, size_t len, size_t bw);

/**
 * Dump title to console.
 */
__Z_EXPORT void __dump_title(int fd, const void *addr, size_t len, size_t bw,
                             const char *, ...);

/**
 * Print given buffer to MVS Console.
 */
__Z_EXPORT void __console(const void *p_in, int len_i);

/**
 * Print formatted data to MVS Console.
 */
__Z_EXPORT int __console_printf(const char *fmt, ...);

/**
 * Finds file in a given path
 * \param [out] out Found path string
 * \param [in] size Max size of path string
 * \param [in] envar Environment variable to search
 * \param [in] file file to search
 * \return returns non-zero if successful, 0 if not found.
 */
__Z_EXPORT int __find_file_in_path(char *out, int size, const char *envvar,
                                   const char *file);

/**
 * Change file descriptor to CCSID.
 * \param [in] fd file descriptor.
 * \param [in] ccsid CCSID.
 * \param [in] bool indicating if txtflag is on
 * \return returns 0 if successful, or -1 on failure.
 */
__Z_EXPORT int __chgfdccsidtxtflag(int fd, unsigned short ccsid, bool txtflag);

/**
 * Change file descriptor to CCSID.
 * \param [in] fd file descriptor.
 * \param [in] ccsid CCSID.
 * \return returns 0 if successful, or -1 on failure.
 */
__Z_EXPORT int __chgfdccsid(int fd, unsigned short ccsid);

/**
 * Change file descriptor to CCSID from a codeset
 * \param [in] fd file descriptor.
 * \param [in] codeset code set
 * \return returns 0 if successful, or -1 on failure.
 */
__Z_EXPORT int __chgfdcodeset(int fd, char* codeset);

/**
 * Change file descriptor to text (819 or controlled via envar)
 * \param [in] fd file descriptor.
 * \return returns 0 if successful, or -1 on failure.
 */
__Z_EXPORT int __setfdtext(int fd);

/**
 * Change file descriptor to binary
 * \param [in] fd file descriptor.
 * \return returns 0 if successful, or -1 on failure.
 */
__Z_EXPORT int __setfdbinary(int fd);

/**
 * Disable auto-conversion on file descriptors
 * \param [in] fd file descriptor.
 * \return returns 0 if successful, or -1 on failure.
 */
__Z_EXPORT int __disableautocvt(int fd);

/**
 * Copy ccsid from source fd to destination fd
 * \param [in] sourcefd file descriptor.
 * \param [in] destfd file descriptor.
 * \return returns 0 if successful, or -1 on failure.
 */
__Z_EXPORT int __copyfdccsid(int sourcefd, int destfd);

/**
 * Get file descriptor CCSID.
 * \param [in] fd file descriptor.
 * \return returns file descriptors ccsid.
 */
__Z_EXPORT int __getfdccsid(int fd);

/**
 * Set file descriptor to the provided CCSID.
 * \param [in] fd file descriptor.
 * \param [in] t_ccsid CCSID.
 * \return returns 0 if successful, or -1 on failure.
 */
__Z_EXPORT int __setfdccsid(int fd, int t_ccsid);

/**
 * Logs memory allocation and release to the file name specified
 * in the environment variable zoslib_config_t.MEMORY_USAGE_LOG_FILE_ENVAR.
 * \param [in] same as C's printf() parameters
 */
__Z_EXPORT void __memprintf(const char *format, ...);

#ifdef __cplusplus
}
#endif
#endif // ZOS_IO_H_
