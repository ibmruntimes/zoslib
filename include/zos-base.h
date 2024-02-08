///////////////////////////////////////////////////////////////////////////////
// Licensed Materials - Property of IBM
// ZOSLIB
// (C) Copyright IBM Corp. 2020. All Rights Reserved.
// US Government Users Restricted Rights - Use, duplication
// or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
///////////////////////////////////////////////////////////////////////////////

#ifndef ZOS_BASE_H_
#define ZOS_BASE_H_

#undef __ZOS_EXT
#define __ZOS_EXT__ 1

#if ' ' != 0x20
#error EBCDIC codeset detected. ZOSLIB is compatible with the ASCII codeset only.
#endif

#include <_Nascii.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/__getipc.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define __ZOS_CC

#include "zos-macros.h"
#include "zos-bpx.h"
#include "zos-char-util.h"
#include "zos-io.h"
#include "zos-savstack.h"
#include "zos-sys-info.h"
#include "zos-tls.h"
#include "zos-getentropy.h"

#define IPC_CLEANUP_ENVAR_DEFAULT "__IPC_CLEANUP"
#define RUNTIME_LIMIT_ENVAR_DEFAULT "__RUNTIMELIMIT"
#define CCSID_GUESS_BUF_SIZE_DEFAULT "__CCSIDGUESSBUFSIZE"
#define UNTAGGED_READ_MODE_DEFAULT "__UNTAGGED_READ_MODE"
#define UNTAGGED_READ_MODE_CCSID1047_DEFAULT "__UNTAGGED_READ_MODE_CCSID1047"
#define MEMORY_USAGE_LOG_FILE_ENVAR_DEFAULT "__MEMORY_USAGE_LOG_FILE"
#define MEMORY_USAGE_LOG_LEVEL_ENVAR_DEFAULT "__MEMORY_USAGE_LOG_LEVEL"

typedef enum {
  __NO_TAG_READ_DEFAULT = 0,
  __NO_TAG_READ_DEFAULT_WITHWARNING = 1,
  __NO_TAG_READ_V6 = 2,
  __NO_TAG_READ_STRICT = 3
} notagread_t;

struct timespec;

extern const char *__zoslib_version;

typedef struct __stack_info {
  void *prev_dsa;
  void *entry_point;
  char entry_name[256];
  int *return_addr;
  int *entry_addr;
  int *stack_addr;
} __stack_info;

#ifdef __cplusplus
#include <bitset>
extern "C" {
#endif

/**
 * Get the environ.
 * \return returns pointer to environment list
 */
__Z_EXPORT char **__get_environ_np(void);

/**
 * Convert environment variables from EBCDIC to ASCII.
 */
__Z_EXPORT void __xfer_env(void);

/**
 * Remove IPC semaphores and shared memory.
 * \param [in] others non-zero value indicates remove IPC not associated
 * with current process.
 */
__Z_EXPORT void __cleanupipc(int others);

/**
 * Retrieves error message from __registerProduct IFAUSAGE macro.
 * \param [in] rc return code from __registerProduct.
 * \return returns error message as C character string.
 */
__Z_EXPORT const char *getIFAUsageErrorString(unsigned long rc);

/**
 * Registers product for SMF 89 Type 1 records using IFAUSAGE macro.
 * \param [in] major_version The major version of Product (e.g. 14)
 * \param [in] product_owner The product owner (e.g. IBM)
 * \param [in] feature_name The feature name (e.g. Node.js)
 * \param [in] product_name The product name (e.g. Node.js for z/OS)
 * \param [in] pid The Product ID (e.g. 5676-SDK)
 * \return returns 0 if successful, non-zero if unsuccessful.
 */
__Z_EXPORT unsigned long long __registerProduct(const char *major_version,
                                                const char *product_owner,
                                                const char *feature_name,
                                                const char *product_name,
                                                const char *pid);

/**
 * Get the Thread ID.
 * \return returns the current thread id
 */
__Z_EXPORT int gettid();

/**
 * Get the main Thread ID.
 * If a process is started with sh -c, main thread id is 0;
 * if started with bash -c, main thread id is 2;
 * if started directly from the shell, main thread id is 1.
 * \return returns the current thread id
 */
__Z_EXPORT int __getMainThreadId();

/**
 * Get the pthread_self() for the main thread.
 * \return returns the current pthread_self() for main thread
 */
__Z_EXPORT pthread_t __getMainThreadSelf();

/**
 * Print backtrace of stack to file descriptor.
 * \param [in] fd file descriptor.
 */
__Z_EXPORT void __display_backtrace(int fd);

/**
 * Enable or disable abort() from calling display_backtrace(). Default is true.
 */
__Z_EXPORT void __set_backtrace_on_abort(bool flag);

/**
 * Generate a backtrace and store into *Buffer.
 * \param [out] buffer Address of location to store backtrace to.
 * \param [in] size Maximum number of bytes to store.
 * \return if successful, returns 0, otherwise -1
 */
__Z_EXPORT int backtrace(void **buffer, int size);

/**
 * Generate a backtrace symbols and store into *Buffer.
 * \param [out] buffer Address of location to store backtrace to.
 * \param [in] size Maximum number of bytes to store.
 * \return if successful, an array of strings, otherwise returns NULL.
 */
__Z_EXPORT char **backtrace_symbols(void *const *buffer, int size);

/**
 * Generate a backtrace symbols and store into *Buffer.
 * \param [out] buffer Address of location to store backtrace to.
 * \param [in] size Maximum number of bytes to store.
 * \param [in] fd file descriptor.
 */
__Z_EXPORT void backtrace_symbols_fd(void *const *buffer, int size, int fd);

/**
 * Generates an SVC 13 abend.
 * \param [in] comp_code Completion code.
 * \param [in] reason_code Reason code.
 * \param [in] flat_byte Flat Byte.
 * \param [in] plist Parameter list.
 */
__Z_EXPORT void __abend(int comp_code, unsigned reason_code, int flat_byte,
                        void *plist);

/**
 * String case comparision that ignores code page.
 * \param [in] a - Character String.
 * \param [in] b - Character String.
 * \param [in] n - Number of bytes to compare.
 * \return if equal, returns 0, otherwise returns non-zero.
 */
__Z_EXPORT int strncasecmp_ignorecp(const char *a, const char *b, size_t n);

/**
 * String case comparision that ignores code page.
 * \param [in] a - null-terminated character string.
 * \param [in] b - null-terminated character string.
 * \return if equal, returns 0, otherwise returns non-zero.
 */
__Z_EXPORT int strcasecmp_ignorecp(const char *a, const char *b);

/**
 * Get program argument list of a given process id
 * \param [out] argc - pointer to store count of the arguments
 * \param [out] argv - pointer to store an array of pointers that point to each
 * argument
 * \param [in] pid - process id to obtain the argc and argv for
 * \note
 * Call free(argv) when done accessing argv.
 * \return On success, returns 0, or -1 on error.
 */
__Z_EXPORT int __getargcv(int *argc, char ***argv, pid_t pid);

/**
 * Get the executable path of a given process id
 * \param [out] path - pointer to the destination array to copy the
 *  null-terminated path to
 * \param [in] pathlen - length of the given array
 * \param [in] pid - process id to obtain the executable path for
 * \return On success, returns 0, or -1 on error.
 */
__Z_EXPORT int __getexepath(char *path, int pathlen, pid_t pid);

/**
 * Get program argument list of the current process
 * \return returns an array of process arguments
 */
__Z_EXPORT char **__getargv(void);

/**
 * Get program argument count of the current process
 * \return returns count of process arguments
 */
__Z_EXPORT int __getargc(void);

/**
 * Get the stack start address for the current thread
 * \return returns the stack start address
 */
__Z_EXPORT int *__get_stack_start();

/**
 * Iterate to next stack dsa based on current dsa
 * \param [in] dsaptr - current dsa entry
 * \param [out] si - stack information of next dsa
 * \return returns the next dsa entry in the chain or 0 if not found
 */
__Z_EXPORT void *__iterate_stack_and_get(void *dsaptr, __stack_info *si);

/**
 * Get next dlcb entry
 * \param [in] last - previous dlcb entry
 * \return [in] returns next dlcb entry
 */
__Z_EXPORT void *__dlcb_next(void *last);

/**
 * Get entry name of given dlcb
 * \param [out] buf - DLL name of given dlcb
 * \param [in] size - maximum number of bytes
 * \param [in] dlcb - current dlcb
 * \return [in] number of bytes written to buf
 */
__Z_EXPORT int __dlcb_entry_name(char *buf, int size, void *dlcb);

/**
 * Get address of dlcb entry
 * \param [in] dlcb - current dlcb
 * \return returns entry address of dlcb
 */
__Z_EXPORT void *__dlcb_entry_addr(void *dlcb);

/**
 * Walk through list of dlcb 
 * \param [in] cb - callback function for each dlcb,
 *  the callback will have the name, the address and data, which is
 *  a copy of whatever value was passed as the second argument, as
 *  input parameters
 * \param [in] data - pass to callback
 * \return returns whatever value was returned by the last call to callback,
 *  if no dlcb is found, return -1
 */
__Z_EXPORT int __dlcb_iterate(int (*cb)(char* name, void* addr, void* data),
                              void *data);

/**
 * Obtain the mach absolute time
 * \return returns mach absolute time
 */
__Z_EXPORT unsigned long __mach_absolute_time(void);

/**
 * Allocate memory in 64-bit virtual storage when size is a megabyte multiple
 * or above 2GB, or in 31-bit storage otherwise, and if none is available,
 * attempt to allocate from 64-bit virtual storage.
 * \param [in] len length in bytes of memory to allocate
 * \param [in] alignment in bytes and applies only to 31-bit storage (64-bit
 *             storage is always megabyte-aligned)
 * \return pointer to the beginning of newly allocated memory, or 0 if
 *         unsuccessful
 */
__Z_EXPORT void *__zalloc(size_t len, size_t alignment);

/**
 * Allocate memory in 64-bit virtual storage when size is a megabyte multiple
 * or above 2GB, or in 31-bit storage (with PAGE_SIZE bytes alignment)
 * otherwise, and if none is available, attempt to allocate from 64-bit virtual
 * storage.
 * \param [in] _ ignored
 * \param [in] len length in bytes of memory to allocate
 * \return pointer to the beginning of newly allocated memory, or MAP_FAILED if
 *         unsuccessful
 * \deprecated This function will be removed once mmap is fully functional
 * (e.g. MAP_ANONYMOUS is supported)
 */
__Z_EXPORT void *anon_mmap(void *_, size_t len);

/**
 * Allocate memory (using __zalloc()) and read into it contents of given file
 * \param [in] len length in bytes of memory to allocate
 * \param [in] filename filename to read
 * \param [in] fd file descriptor
 * \param [in] offset offset in bytes into the file to read
 * \return pointer to the beginning of newly allocated memory, or 0 if
 *         unsuccessful
 */
__Z_EXPORT void *__zalloc_for_fd(size_t len, const char *filename, int fd,
                                 off_t offset);

/**
 * Allocate memory (using __zalloc()) and read into it contents of given file
 * at the given offset.
 * \param [in] _ ignored
 * \param [in] len length in bytes of memory map
 * \param [in] prot protection bits
 * \param [in] flags mmap flags
 * \param [in] filename filename to read
 * \param [in] fd file descriptor
 * \param [in] offset offset in bytes into the file to read
 * \return pointer to the beginning of newly allocated memory, or MAP_FAILED if
 *         unsuccessful
 * \deprecated This function will be removed once mmap is fully functional
 * (e.g. MAP_ANONYMOUS is supported), in which case mapped memory would need to
 * be converted to ASCII if the file contains EBCDIC.
 */
__Z_EXPORT void *roanon_mmap(void *_, size_t len, int prot, int flags,
                  const char *filename, int fd, off_t offset);
/**
 * Deallocate memory
 * \param [in] addr start address of memory
 * \param [in] len length in bytes
 * \return returns 0 if successful, -1 if unsuccessful
 */
__Z_EXPORT int __zfree(void *addr, int len);

/**
 * Deallocate memory
 * \param [in] addr start address of memory
 * \param [in] len length in bytes
 * \return returns 0 if successful, -1 if unsuccessful
 * \deprecated This function will be removed once mmap is fully functional
 * (e.g. MAP_ANONYMOUS is supported)
 */
__Z_EXPORT int anon_munmap(void *addr, size_t len);

/**
 * Check if an LE function is present in the LE vector table
 * \param [in] addr address to LE function
 * \param [out] funcname pointer to string that will hold the function name
 * \param [out] max length of string corresponding to funcname
 * \return returns 1 if successful, 0 if unsuccessful.
 */
int __check_le_func(void *addr, char *funcname, size_t len);

/**
 * Suspend the calling thread until any one of a set of events has occurred
 * or until a specified amount of time has passed.
 * \param [in] secs seconds to suspend
 * \param [in] nsecs nanoseconds to suspend
 * \param [in] event_list events that will trigger thread to resume (CW_INTRPT
 *  or CW_CONDVAR)
 * \param [out] secs_rem seconds remaining
 * \param [out] nsecs_rem nanoseconds remaining
 * \return returns 0 if successful, -1 if unsuccessful.
 */
__Z_EXPORT int __cond_timed_wait(unsigned int secs, unsigned int nsecs,
                      unsigned int event_list, unsigned int *secs_rem,
                      unsigned int *nsecs_rem);

enum COND_TIME_WAIT_CONSTANTS { CW_INTRPT = 1, CW_CONDVAR = 32 };

/**
 * Fill a buffer with random bytes
 * \param [out] buffer to store random bytes to.
 * \param [in] number of random bytes to generate.
 * \return On success, returns 0, or -1 on error.
 */
__Z_EXPORT int __getentropy(void *buffer, size_t length);

/**
 * Return the LE version as a string in the format of
 * "Product %d%s Version %d Release %d Modification %d" 
 */
__Z_EXPORT char* __get_le_version(void);

/**
 * Prints the build version of the library
 */
__Z_EXPORT void __build_version(void);

/**
 * Attempts to a close a socket for a period of time
 * \param [in] socket socket handle
 * \param [in] secs number of seconds to attempt the close
 */
__Z_EXPORT void __tcp_clear_to_close(int socket, unsigned int secs);

/**
 * Returns the overview structure of IPCQPROC
 * \param [out] info address of allocated IPCQPROC structure
 * \return On success, returns 0, or -1 on error.
 */
__Z_EXPORT int get_ipcs_overview(IPCQPROC *info);

/**
 * Prints zoslib help information to specified FILE pointer
 * \param [in] FILE pointer to write to
 * \param [in] title header, specify NULL for default
 * \return On success, returns 0, or < 0 on error.
 */
__Z_EXPORT int __print_zoslib_help(FILE *fp, const char *title);

typedef struct __cpu_relax_workarea {
  void *sfaddr;
  unsigned long t0;
} __crwa_t;

/**TODO(itodorov) - zos: document these interfaces**/
__Z_EXPORT void __cpu_relax(__crwa_t *);

/**TODO(itodorov) - zos: document these interfaces **/
__Z_EXPORT int __testread(const void *location);
__Z_EXPORT void __tb(void);

__Z_EXPORT notagread_t __get_no_tag_read_behaviour();
__Z_EXPORT int __get_no_tag_ignore_ccsid1047();

#ifdef __cplusplus
/**
 * Configuration for zoslib library
 */
typedef struct __Z_EXPORT zoslib_config {
  /**
   * String to indicate the envar to be used to toggle IPC cleanup.
   */
  const char *IPC_CLEANUP_ENVAR = IPC_CLEANUP_ENVAR_DEFAULT;
  /**
   * String to indicate the envar to be used to toggle runtime limit.
   */
  const char *RUNTIME_LIMIT_ENVAR = RUNTIME_LIMIT_ENVAR_DEFAULT;
  /**
   * String to indicate the envar to be used to toggle ccsid guess buf size in
   * bytes.
   */
  const char *CCSID_GUESS_BUF_SIZE_ENVAR = CCSID_GUESS_BUF_SIZE_DEFAULT;
  /**
   * String to indicate the envar to be used to toggle the untagged read mode.
   */
  const char *UNTAGGED_READ_MODE_ENVAR = UNTAGGED_READ_MODE_DEFAULT;
  /**
   * String to indicate the envar to be used to toggle the untagged 1047 read
   * mode.
   */
  const char *UNTAGGED_READ_MODE_CCSID1047_ENVAR =
      UNTAGGED_READ_MODE_CCSID1047_DEFAULT;
  /**
   * String to indicate the envar to be used to set the name of the log file,
   * including 'stdout' or 'stderr', to which diagnostic messages for memory
   * allocation and release are to be written.
   */
  const char *MEMORY_USAGE_LOG_FILE_ENVAR = MEMORY_USAGE_LOG_FILE_ENVAR_DEFAULT;
  /**
   * String to indicate the envar to be used to specify the level of details
   * to display when memory is allocated or freed.
   */
  const char *MEMORY_USAGE_LOG_LEVEL_ENVAR =
              MEMORY_USAGE_LOG_LEVEL_ENVAR_DEFAULT;
} zoslib_config_t;

/**
 * Initialize zoslib library
 * \param [in] config struct to configure zoslib.
 */
__Z_EXPORT void init_zoslib(const zoslib_config_t config = {});

#else
/**
 * Configuration for zoslib library
 */
typedef struct __Z_EXPORT zoslib_config {
  /**
   * string to indicate the envar to be used to toggle IPC cleanup
   */
  const char *IPC_CLEANUP_ENVAR;
  /**
   * string to indicate the envar to be used to toggle runtime limit
   */
  const char *RUNTIME_LIMIT_ENVAR;
  /**
   * string to indicate the envar to be used to toggle ccsid guess buf size in
   * bytes
   */
  const char *CCSID_GUESS_BUF_SIZE_ENVAR;
  /**
   * string to indicate the envar to be used to toggle the untagged read mode
   */
  const char *UNTAGGED_READ_MODE_ENVAR;
  /**
   * string to indicate the envar to be used to toggle the untagged 1047 read
   * mode
   */
  const char *UNTAGGED_READ_MODE_CCSID1047_ENVAR;
  /**
   * String to indicate the envar to be used to set the name of the log file,
   * including 'stdout' or 'stderr', to which diagnostic messages for memory
   * allocation and release are to be written.
   */
  const char *MEMORY_USAGE_LOG_FILE_ENVAR;
  /**
   * String to indicate the envar to be used to specify the level of details
   * to display when memory is allocated or freed.
   */
  const char *MEMORY_USAGE_LOG_LEVEL_ENVAR;
} zoslib_config_t;

/**
 * Initialize zoslib library
 * \param [in] config struct to configure zoslib.
 */
__Z_EXPORT void init_zoslib(const zoslib_config_t config);

#endif // __cplusplus

/**
 * Initialize the struct used to configure zoslib with default values.
 * \param [in] config struct to configure zoslib.
 */
__Z_EXPORT void init_zoslib_config(zoslib_config_t *const config);

/**
 * Updates the zoslib global variables associated with the zoslib environment
 * variables \param [in] envar environment variable to update, specify NULL to
 * update all \return 0 for success, or -1 for failure
 */
__Z_EXPORT int __update_envar_settings(const char *envar);

/**
 * Gets the LE libvec base address
 * \return libvec base address
 */
unsigned long __get_libvec_base(void);

/**
 * Changes the names of one or more of the environment variables zoslib uses
 * \param [in] zoslib_confit_t structure that defines the new environment
 * variable name(s) \return 0 for success, or -1 for failure
 */
__Z_EXPORT int __update_envar_names(zoslib_config_t *const config);

/**
 * Tell zoslib that the main process is terminating, for its diagnostics.
 *
 */
__Z_EXPORT void __mainTerminating();

/**
 * Returns the program's directory as an absolute path
 */
__Z_EXPORT char* __getprogramdir();

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
__Z_EXPORT void init_zoslib_config(zoslib_config_t &config);

#include <exception>
#include <map>
#include <string>
#include <unordered_map>

inline bool operator==(const pthread_t &_a, const pthread_t &_b) {
  return _a.__ == _b.__;
}
inline bool operator!=(const pthread_t &_a, const pthread_t &_b) {
  return _a.__ != _b.__;
}
inline bool operator<=(const pthread_t &_a, const pthread_t &_b) {
  return _a.__ <= _b.__;
}
inline bool operator>=(const pthread_t &_a, const pthread_t &_b) {
  return _a.__ >= _b.__;
}
inline bool operator<(const pthread_t &_a, const pthread_t &_b) {
  return _a.__ < _b.__;
}
inline bool operator>(const pthread_t &_a, const pthread_t &_b) {
  return _a.__ > _b.__;
}
inline bool operator==(const pthread_t &_a, const int _b) {
  return _a.__ == static_cast<unsigned long long>(_b);
}
inline bool operator!=(const pthread_t &_a, const int _b) {
  return _a.__ != static_cast<unsigned long long>(_b);
}

struct zoslibEnvar {
  std::string envarName;
  std::string envarValue;

  zoslibEnvar(std::string name, std::string value)
      : envarName(name), envarValue(value) {}

  bool operator<(const zoslibEnvar &t) const {
    return std::tie(envarName, envarValue) <
           std::tie(t.envarName, t.envarValue);
  }
};

class __zinit {
  int mode;
  int cvstate;
  std::terminate_handler _th;

public:
  zoslib_config_t config;
  std::map<zoslibEnvar, std::string> envarHelpMap;

public:
  __zinit();
  ~__zinit();

  int initialize(const zoslib_config_t &config);
  bool isValidZOSLIBEnvar(std::string envar);
  int setEnvarHelpMap(void);
  void populateLEFunctionPointers(void);

  void __abort() { _th(); }

private:
  void del_instance();
};

struct __Z_EXPORT __init_zoslib {
  __init_zoslib(const zoslib_config_t &config = {});
};

/**
 * Subtract 1 from the given bitset.
 * \param [in] bitset
 * \param [out] bitset - 1
 * \return bitset - 1.
 */
template <std::size_t N> __Z_EXPORT std::bitset<N>
                                    __subtractOne(std::bitset<N> bs) {
  // Flip bits from rightmost bit till and including the first 1:
  for (int i=0; i<bs.size(); i++) {
    if (bs[i]) {
      bs[i] = 0b0;
      break;
    } else {
      bs[i] = 0b1;
    }
  }
  return bs;
}

/**
 * Add 1 to the given bitset.
 * \param [in] bitset
 * \param [out] bitset + 1
 * \return bitset + 1.
 */
template <std::size_t N> __Z_EXPORT std::bitset<N>
                                    __addOne(std::bitset<N> bs) {
  // Flip bits from rightmost bit till and including the first 0:
  for (int i=0; i<bs.size(); i++) {
    if (!bs[i]) {
      bs[i] = 0b1;
      break;
    } else {
      bs[i] = 0b0;
    }
  }
  return bs;
}

__zinit* __get_instance();

#endif // __cplusplus
#endif // ZOS_BASE_H_
