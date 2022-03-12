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
#include <time.h>
#include <unistd.h>

#define __ZOS_CC

#include "zos-bpx.h"
#include "zos-char-util.h"
#include "zos-io.h"
#include "zos-savstack.h"
#include "zos-sys-info.h"
#include "zos-tls.h"
#include "zos-getentropy.h"

#define IPC_CLEANUP_ENVAR_DEFAULT "__IPC_CLEANUP"
#define DEBUG_ENVAR_DEFAULT "__RUNDEBUG"
#define RUNTIME_LIMIT_ENVAR_DEFAULT "__RUNTIMELIMIT"
#define FORKMAX_ENVAR_DEFAULT "__FORKMAX"
#define CCSID_GUESS_BUF_SIZE_DEFAULT "__CCSIDGUESSBUFSIZE"
#define UNTAGGED_READ_MODE_DEFAULT "__UNTAGGED_READ_MODE"
#define UNTAGGED_READ_MODE_CCSID1047_DEFAULT "__UNTAGGED_READ_MODE_CCSID1047"

typedef enum {
  __NO_TAG_READ_DEFAULT = 0,
  __NO_TAG_READ_DEFAULT_WITHWARNING = 1,
  __NO_TAG_READ_V6 = 2,
  __NO_TAG_READ_STRICT = 3
} notagread_t;

struct timespec;

typedef enum {
  CLOCK_REALTIME,
  CLOCK_MONOTONIC,
  CLOCK_HIGHRES,
  CLOCK_THREAD_CPUTIME_ID
} clockid_t;

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
extern "C" {
#endif

/**
 * Get current time of clock.
 * \param [in] clk_id Clock id.
 * \param [out] tp structure to store the current time to.
 * \return return 0 for success, or -1 for failure.
 */
int clock_gettime(clockid_t clk_id, struct timespec *tp);

/**
 * Get the environ.
 * \return returns pointer to environment list
 */
char **__get_environ_np(void);

/**
 * Convert environment variables from EBCDIC to ASCII.
 */
void __xfer_env(void);

/**
 * Remove IPC semaphores and shared memory.
 * \param [in] others non-zero value indicates remove IPC not associated
 * with current process.
 */
void __cleanupipc(int others);

/**
 * Retrieves error message from __registerProduct IFAUSAGE macro.
 * \param [in] rc return code from __registerProduct.
 * \return returns error message as C character string.
 */
const char *getIFAUsageErrorString(unsigned long rc);

/**
 * Registers product for SMF 89 Type 1 records using IFAUSAGE macro.
 * \param [in] major_version The major version of Product (e.g. 14)
 * \param [in] product_owner The product owner (e.g. IBM)
 * \param [in] feature_name The feature name (e.g. Node.js)
 * \param [in] product_name The product name (e.g. Node.js for z/OS)
 * \param [in] pid The Product ID (e.g. 5676-SDK)
 * \return returns 0 if successful, non-zero if unsuccessful.
 */
unsigned long long __registerProduct(const char *major_version,
                                     const char *product_owner,
                                     const char *feature_name,
                                     const char *product_name, const char *pid);

/**
 * Get the Thread ID.
 * \return returns the current thread id
 */
int gettid();

/**
 * Print backtrace of stack to file descriptor.
 * \param [in] fd file descriptor.
 */
void __display_backtrace(int fd);

/**
 * Enable or disable abort() from calling display_backtrace(). Default is true.
 */
void __set_backtrace_on_abort(bool flag);

/**
 * Execute a file.
 * \param [in] name used to construct a pathname that identifies the new
 *  process image file.
 * \param [in] argv an array of character pointers to NULL-terminated strings.
 * \param [in] envp an array of character pointers to NULL-terminated strings.
 * \return if successful, it doesn't return; otherwise, it returns -1 and sets
 *  errno.
 */
int execvpe(const char *name, char *const argv[], char *const envp[]);

/**
 * Generate a backtrace and store into *Buffer.
 * \param [out] buffer Address of location to store backtrace to.
 * \param [in] size Maximum number of bytes to store.
 * \return if successful, returns 0, otherwise -1
 */
int backtrace(void **buffer, int size);

/**
 * Generate a backtrace symbols and store into *Buffer.
 * \param [out] buffer Address of location to store backtrace to.
 * \param [in] size Maximum number of bytes to store.
 * \return if successful, an array of strings, otherwise returns NULL.
 */
char **backtrace_symbols(void *const *buffer, int size);

/**
 * Generate a backtrace symbols and store into *Buffer.
 * \param [out] buffer Address of location to store backtrace to.
 * \param [in] size Maximum number of bytes to store.
 * \param [in] fd file descriptor.
 */
void backtrace_symbols_fd(void *const *buffer, int size, int fd);

/**
 * Generates an SVC 13 abend.
 * \param [in] comp_code Completion code.
 * \param [in] reason_code Reason code.
 * \param [in] flat_byte Flat Byte.
 * \param [in] plist Parameter list.
 */
void __abend(int comp_code, unsigned reason_code, int flat_byte, void *plist);

/**
 * String case comparision that ignores code page.
 * \param [in] a - Character String.
 * \param [in] b - Character String.
 * \param [in] n - Number of bytes to compare.
 * \return if equal, returns 0, otherwise returns non-zero.
 */
int strncasecmp_ignorecp(const char *a, const char *b, size_t n);

/**
 * String case comparision that ignores code page.
 * \param [in] a - null-terminated character string.
 * \param [in] b - null-terminated character string.
 * \return if equal, returns 0, otherwise returns non-zero.
 */
int strcasecmp_ignorecp(const char *a, const char *b);

/**
 * Indicates if zoslib is in debug mode
 * \return returns current debug mode
 */
int __indebug(void);

/**
 * Activates debug mode
 */
void __setdebug(int);

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
int __getargcv(int *argc, char ***argv, pid_t pid);

/**
 * Get the executable path of a given process id
 * \param [out] path - pointer to the destination array to copy the
 *  null-terminated path to
 * \param [in] pathlen - length of the given array
 * \param [in] pid - process id to obtain the executable path for
 * \return On success, returns 0, or -1 on error.
 */
int __getexepath(char *path, int pathlen, pid_t pid);

/**
 * Get program argument list of the current process
 * \return returns an array of process arguments
 */
char **__getargv(void);

/**
 * Get program argument count of the current process
 * \return returns count of process arguments
 */
int __getargc(void);

/**
 * Get the stack start address for the current thread
 * \return returns the stack start address
 */
int *__get_stack_start();

/**
 * Iterate to next stack dsa based on current dsa
 * \param [in] dsaptr - current dsa entry
 * \param [out] si - stack information of next dsa
 * \return returns the next dsa entry in the chain or 0 if not found
 */
void *__iterate_stack_and_get(void *dsaptr, __stack_info *si);

/**
 * Check if STFLE (STORE FACILITY LIST EXTENDED) instruction is available
 * \return true if the STFLE instruction is available
 */
bool __is_stfle_available();

/**
 * Get next dlcb entry
 * \param [in] last - previous dlcb entry
 * \return [in] returns next dlcb entry
 */
void *__dlcb_next(void *last);

/**
 * Get entry name of given dlcb
 * \param [out] buf - DLL name of given dlcb
 * \param [in] size - maximum number of bytes
 * \param [in] dlcb - current dlcb
 * \return [in] number of bytes written to buf
 */
int __dlcb_entry_name(char *buf, int size, void *dlcb);

/**
 * Get address of dlcb entry
 * \param [in] dlcb - current dlcb
 * \return returns entry address of dlcb
 */
void *__dlcb_entry_addr(void *dlcb);

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
int __dlcb_iterate(int (*cb)(char* name, void* addr, void* data), void *data);

/**
 * Obtain the mach absolute time
 * \return returns mach absolute time
 */
unsigned long __mach_absolute_time(void);

/**
 * Generate an anonymous memory map
 * \param [in] _ ignored
 * \param [in] len length in bytes of memory map
 * \return returns start address of anonymous memory map
 */
void *anon_mmap(void *_, size_t len);

/**
 * Generate a read only anonymous memory map for a given file
 * \param [in] _ ignored
 * \param [in] len length in bytes of memory map
 * \param [in] prot protection bits
 * \param [in] flags mmap flags
 * \param [in] filename filename to read
 * \param [in] filedes file descriptor
 * \return returns start address of anonymous memory map
 */
void *roanon_mmap(void *_, size_t len, int prot, int flags,
                  const char *filename, int fildes, off_t off);

/**
 * Deallocates memory map
 * \param [in] addr start address of memory map
 * \param [in] len length in bytes
 * \return returns 0 if successful, -1 if unsuccessful.
 */
int anon_munmap(void *addr, size_t len);

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
int __cond_timed_wait(unsigned int secs, unsigned int nsecs,
                      unsigned int event_list, unsigned int *secs_rem,
                      unsigned int *nsecs_rem);

enum COND_TIME_WAIT_CONSTANTS { CW_INTRPT = 1, CW_CONDVAR = 32 };

/**
 * Create a child process
 * \return On success, the PID of the child process is returned in the
 *  parent, and 0 is returned in the child.  On failure, -1 is returned in the
 *  parent, no child process is created, and errno is set appropriately.
 */
int __fork(void);

/**
 * Fill a buffer with random bytes
 * \param [out] buffer to store random bytes to.
 * \param [in] number of random bytes to generate.
 * \return On success, returns 0, or -1 on error.
 */
int getentropy(void *buffer, size_t length);

/**
 * Return the LE version as a string in the format of
 * "Product %d%s Version %d Release %d Modification %d" 
 */
char* __get_le_version(void);

/**
 * Prints the build version of the library
 */
void __build_version(void);

/**
 * Determine the length of a fixed-size string
 * \param [in] str fixed-size character string
 * \param [in] maxlen maximum # of bytes to traverse
 * \return returns the length of the string
 */
size_t strnlen(const char *str, size_t maxlen);

/**
 * Attempts to a close a socket for a period of time
 * \param [in] socket socket handle
 * \param [in] secs number of seconds to attempt the close
 */
void __tcp_clear_to_close(int socket, unsigned int secs);

/**
 * Returns the overview structure of IPCQPROC
 * \param [out] info address of allocated IPCQPROC structure
 * \return On success, returns 0, or -1 on error.
 */
int get_ipcs_overview(IPCQPROC *info);

/**
 * Prints zoslib help information to specified FILE pointer
 * \param [in] FILE pointer to write to
 * \param [in] title header, specify NULL for default
 * \return On success, returns 0, or < 0 on error.
 */
int __print_zoslib_help(FILE *fp, const char *title);

typedef struct __cpu_relax_workarea {
  void *sfaddr;
  unsigned long t0;
} __crwa_t;

/**TODO(itodorov) - zos: document these interfaces**/
void __cpu_relax(__crwa_t *);

/**TODO(itodorov) - zos: document these interfaces **/
int __testread(const void *location);
void __tb(void);

notagread_t __get_no_tag_read_behaviour();
int __get_no_tag_ignore_ccsid1047();

#ifdef __cplusplus
/**
 * Configuration for zoslib library
 */
typedef struct zoslib_config {
  /**
   * string to indicate the envar to be used to toggle IPC cleanup
   */
  const char *IPC_CLEANUP_ENVAR = IPC_CLEANUP_ENVAR_DEFAULT;
  /**
   * string to indicate the envar to be used to toggle debug mode
   */
  const char *DEBUG_ENVAR = DEBUG_ENVAR_DEFAULT;
  /**
   * string to indicate the envar to be used to toggle runtime limit
   */
  const char *RUNTIME_LIMIT_ENVAR = RUNTIME_LIMIT_ENVAR_DEFAULT;
  /**
   * string to indicate the envar to be used to toggle max number of forks
   */
  const char *FORKMAX_ENVAR = FORKMAX_ENVAR_DEFAULT;
  /**
   * string to indicate the envar to be used to toggle ccsid guess buf size in
   * bytes
   */
  const char *CCSID_GUESS_BUF_SIZE_ENVAR = CCSID_GUESS_BUF_SIZE_DEFAULT;
  /**
   * string to indicate the envar to be used to toggle the untagged read mode
   */
  const char *UNTAGGED_READ_MODE_ENVAR = UNTAGGED_READ_MODE_DEFAULT;
  /**
   * string to indicate the envar to be used to toggle the untagged 1047 read
   * mode
   */
  const char *UNTAGGED_READ_MODE_CCSID1047_ENVAR =
      UNTAGGED_READ_MODE_CCSID1047_DEFAULT;
} zoslib_config_t;

/**
 * Initialize zoslib library
 * \param [in] config struct to configure zoslib.
 */
void init_zoslib(const zoslib_config_t config = {});

#else
/**
 * Configuration for zoslib library
 */
typedef struct zoslib_config {
  /**
   * string to indicate the envar to be used to toggle IPC cleanup
   */
  const char *IPC_CLEANUP_ENVAR;
  /**
   * string to indicate the envar to be used to toggle debug mode
   */
  const char *DEBUG_ENVAR;
  /**
   * string to indicate the envar to be used to toggle runtime limit
   */
  const char *RUNTIME_LIMIT_ENVAR;
  /**
   * string to indicate the envar to be used to toggle max number of forks
   */
  const char *FORKMAX_ENVAR;
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
} zoslib_config_t;

/**
 * Initialize zoslib library
 * \param [in] config struct to configure zoslib.
 */
void init_zoslib(const zoslib_config_t config);

#endif // __cplusplus

/**
 * Initialize the struct used to configure zoslib with default values.
 * \param [in] config struct to configure zoslib.
 */
void init_zoslib_config(zoslib_config_t *const config);

/**
 * Suspends the execution of the calling thread until either at least the
 * time specified in *req has elapsed, an event occurs, or a signal arrives.
 * \param [in] req struct used to specify intervals of time with nanosecond
 *  precision
 * \param [out] rem the remaining time if the call is interrupted
 */
int nanosleep(const struct timespec *req, struct timespec *rem);

/**
 * Changes the access and modification times of a file in the same way as
 * lutimes, with the difference that microsecond precision is not supported.
 * \param [in] filename the path to file
 * \param [in] tv two structs used to specify the new times
 */
int __lutimes(const char *filename, const struct timeval tv[2]);

/**
 * Updates the zoslib global variables associated with the zoslib environment
 * variables \param [in] envar environment variable to update, specify NULL to
 * update all \return 0 for success, or -1 for failure
 */
int __update_envar_settings(const char *envar);

/**
 * Changes the names of one or more of the environment variables zoslib uses
 * \param [in] zoslib_confit_t structure that defines the new environment
 * variable name(s) \return 0 for success, or -1 for failure
 */
int __update_envar_names(zoslib_config_t *const config);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
void init_zoslib_config(zoslib_config_t &config);

#include <exception>
#include <map>
#include <string>

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
  int __forked;
  static __zinit *instance;

public:
  int forkmax;
  int *forkcurr;
  int shmid;
  zoslib_config_t config;
  std::map<zoslibEnvar, std::string> envarHelpMap;

public:
  __zinit(const zoslib_config_t &config);

  bool isValidZOSLIBEnvar(std::string envar);

  static __zinit *init(const zoslib_config_t &config) {
    instance = new __zinit(config);
    instance->initialize();
    return instance;
  }

  int initialize(void);
  int setEnvarHelpMap(void);

  static __zinit *getInstance() { return instance; }

  int forked(int newvalue) {
    int old = __forked;
    __forked = newvalue;
    return old;
  }

  int get_forkmax(void) { return forkmax; }

  int inc_forkcount(void) {
    if (0 == forkmax || 0 == shmid)
      return 0;
    int original;
    int new_value;

    do {
      original = *forkcurr;
      new_value = original + 1;
      __asm volatile(" cs %0,%2,%1 \n "
                     : "+r"(original), "+m"(*forkcurr)
                     : "r"(new_value)
                     :);
    } while (original != (new_value - 1));
    return new_value;
  }
  int dec_forkcount(void) {
    if (0 == forkmax || 0 == shmid)
      return 0;
    int original;
    int new_value;

    do {
      original = *forkcurr;
      if (original == 0)
        return 0;
      new_value = original - 1;
      __asm volatile(" cs %0,%2,%1 \n "
                     : "+r"(original), "+m"(*forkcurr)
                     : "r"(new_value)
                     :);
    } while (original != (new_value - 1));
    return new_value;
  }
  int shmid_value(void) { return shmid; }

  ~__zinit() {
    if (_CVTSTATE_OFF == cvstate) {
      __ae_autoconvert_state(cvstate);
    }
    __ae_thread_swapmode(mode);
    if (shmid != 0) {
      if (__forked)
        dec_forkcount();
      shmdt(forkcurr);
      shmctl(shmid, IPC_RMID, 0);
    }
    __cleanupipc(0);
  }
  void __abort() { _th(); }
};

struct __init_zoslib {
  __init_zoslib(const zoslib_config_t &config = {}) { __zinit::init(config); }
};

#endif // __cplusplus
#endif // ZOS_BASE_H_
