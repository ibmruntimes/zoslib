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
 * \return returns 0 if successful, or -1 on failure.
 */
__Z_EXPORT int __chgfdccsid(int fd, unsigned short ccsid);

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
 * Logs memory diagnostics to the file name specified in the environment variable
 * zoslib_config_t.MEMORY_USAGE_LOG_FILE_ENVAR.
 * \param [in] same as C's printf() parameters
 */
__Z_EXPORT void __memprintf(const char *format, ...);

/**
 * Returns 1 if logging of memory diagnostics is enabled.
 */
__Z_EXPORT int __doLogMemoryUsage();

/**
 * Returns the file name, including "stdout" or "stderr", used to log memory
 * diagnostics to.
 */
__Z_EXPORT char *__getMemoryUsageLogFile();

/**
 * Returns the fileno to which memory diagnostics is written (use for
 * instance in a `__display_backtrace(__getLogMemoryFileNo()); call).
 */
__Z_EXPORT int __getLogMemoryFileNo();

/**
 * Returns 1 if all messages from memory diagnostics are enabled.
 */
__Z_EXPORT int __doLogMemoryAll();

/**
 * Returns 1 if only warnings from memory diagnostics are enabled.
 * Errors are always included if memory diangostics is enabled.
 */
__Z_EXPORT int __doLogMemoryWarning();

#ifdef __cplusplus
}
#endif
#endif // ZOS_IO_H_
