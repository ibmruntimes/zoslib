#ifndef __ZOS_H_
#define __ZOS_H_
#undef __ZOS_EXT
#define __ZOS_EXT__ 1
//-------------------------------------------------------------------------------------
//
// external interface:
//
#include <_Nascii.h>
#include <stdarg.h>
#define __ZOS_CC
#ifdef __cplusplus
extern "C" {
#endif
#ifndef __size_t
typedef unsigned long size_t;
#define __size_t 1
#endif

#define MAJOR_VERSION 1
#define MINOR_VERSION 0
#define PATCH_VERSION 0

typedef enum {
  CLOCK_REALTIME,
  CLOCK_MONOTONIC,
  CLOCK_HIGHRES,
  CLOCK_THREAD_CPUTIME_ID
} clockid_t;

typedef struct __sem {
  volatile unsigned int value;
  volatile unsigned int id; // 0 for non shared (thread), pid for share
  volatile unsigned int waitcnt;
  pthread_mutex_t mutex;
  pthread_cond_t cond;
} ____sem_t;

typedef struct Semaphore {
  ____sem_t *_s;
} __sem_t;
struct timespec;
extern void* _convert_e2a(void* dst, const void* src, size_t size);
extern void* _convert_a2e(void* dst, const void* src, size_t size);
int clock_gettime(clockid_t clk_id, struct timespec* tp);
extern char** __get_environ_np(void);
extern void __xfer_env(void);
extern int __chgfdccsid(int fd, unsigned short ccsid);
extern int __getfdccsid(int fd);
extern int __setfdccsid(int fd, int t_ccsid);
extern unsigned long long __registerProduct(const char *major_version,
                                     const char *product_owner,
                                     const char *feature_name,
                                     const char *product_name,
                                     const char *pid);
extern size_t __e2a_l(char* bufptr, size_t szLen);
extern size_t __a2e_l(char* bufptr, size_t szLen);
extern size_t __e2a_s(char* string);
extern size_t __a2e_s(char* string);
extern int gettid();
extern int dprintf(int fd, const char*, ...);
extern int vdprintf(int fd, const char*, va_list ap);
extern void __xfer_env(void);
extern void __dump(int fd, const void* addr, size_t len, size_t bw);
extern void __dump_title(
    int fd, const void* addr, size_t len, size_t bw, const char*, ...);
extern void __display_backtrace(int fd);
extern int execvpe(const char* name, char* const argv[], char* const envp[]);
extern int backtrace(void** buffer, int size);
extern char** backtrace_symbols(void* const* buffer, int size);
extern void backtrace_symbols_fd(void* const* buffer, int size, int fd);
extern void __fdinfo(int fd);
extern void __abend(int comp_code,
                    unsigned reason_code,
                    int flat_byte,
                    void* plist);
extern int strncasecmp_ignorecp(const char* a, const char* b, size_t n);
extern int strcasecmp_ignorecp(const char* a, const char* b);
extern int __guess_ae(const void* src, size_t size);
extern int conv_utf8_utf16(char*, size_t, const char*, size_t);
extern int conv_utf16_utf8(char*, size_t, const char*, size_t);
extern int __console_printf(const char* fmt, ...);
extern int __indebug(void);
extern void __setdebug(int);
extern int __getargcv(int *argc, char ***argv, pid_t pid);
extern char** __getargv(void);
extern int __getargc(void);

extern void* __dlcb_next(void* last);
extern int __dlcb_entry_name(char* buf, int size, void* dlcb);
extern void* __dlcb_entry_addr(void* dlcb);
extern int __find_file_in_path(char* out,
                               int size,
                               const char* envvar,
                               const char* file);
extern void __listdll(int fd);
extern int __file_needs_conversion(int fd);
extern int __file_needs_conversion_init(const char* name, int fd);
extern void __fd_close(int fd);

extern unsigned long __mach_absolute_time(void);
extern void* anon_mmap(void* _, size_t len);
extern void* roanon_mmap(void* _, size_t len, int prot, int flags, const char* filename, int fildes, off_t off);
extern int anon_munmap(void* addr, size_t len);
extern int __cond_timed_wait(unsigned int secs,
                             unsigned int nsecs,
                             unsigned int event_list,
                             unsigned int* secs_rem,
                             unsigned int* nsecs_rem);

enum COND_TIME_WAIT_CONSTANTS { CW_INTRPT = 1, CW_CONDVAR = 32 };

extern int __fork(void);
struct __tlsanchor;
extern struct __tlsanchor* __tlsvaranchor_create(size_t sz);
extern void __tlsvaranchor_destroy(struct __tlsanchor* anchor);
extern void* __tlsPtrFromAnchor(struct __tlsanchor* anchor, const void*);
extern int __testread(const void* location);
extern void __tb(void);
extern int getentropy(void* buffer, size_t length);
extern void __build_version(void);
extern size_t strnlen(const char* str, size_t maxlen);
extern void __tcp_clear_to_close(int socket, unsigned int secs);

typedef struct __cpu_relax_workarea {
  void *sfaddr;
  unsigned long t0;
} __crwa_t;

extern void __cpu_relax(__crwa_t *);
extern int __sem_init(__sem_t *s0, int shared, unsigned int val);
extern int __sem_post(__sem_t *s0);
extern int __sem_trywait(____sem_t *s0);
extern int __sem_timedwait(____sem_t *s0, const struct timespec *abs_timeout);
extern int __sem_wait(__sem_t *s0);
extern int __sem_getvalue(__sem_t *s0, int *sval);
extern int __sem_destroy(__sem_t *s0);

#ifdef __cplusplus
}
#endif

#define _str_e2a(_str)                                                         \
  ({                                                                           \
    const char* src = (const char*)(_str);                                     \
    int len = strlen(src) + 1;                                                 \
    char* tgt = (char*)alloca(len);                                            \
    (char*)_convert_e2a(tgt, src, len);                                        \
  })

#define AEWRAP(_rc, _x)                                                        \
  (__isASCII() ? ((_rc) = (_x), 0)                                             \
               : (__ae_thread_swapmode(__AE_ASCII_MODE),                       \
                  ((_rc) = (_x)),                                              \
                  __ae_thread_swapmode(__AE_EBCDIC_MODE),                      \
                  1))

#define AEWRAP_VOID(_x)                                                        \
  (__isASCII() ? ((_x), 0)                                                     \
               : (__ae_thread_swapmode(__AE_ASCII_MODE),                       \
                  (_x),                                                        \
                  __ae_thread_swapmode(__AE_EBCDIC_MODE),                      \
                  1))

#ifdef __cplusplus
class __auto_ascii {
  int ascii_mode;

 public:
  __auto_ascii();
  ~__auto_ascii();
};

class __conv_off {
  int convert_state;

 public:
  __conv_off();
  ~__conv_off();
};

template <typename T>
class __tlssim {
  struct __tlsanchor* anchor;
  T v;

 public:
  __tlssim(const T& initvalue) : v(initvalue) {
    anchor = __tlsvaranchor_create(sizeof(T));
  }
  ~__tlssim() { __tlsvaranchor_destroy(anchor); }
  T* access(void) { return static_cast<T*>(__tlsPtrFromAnchor(anchor, &v)); }
};

class __zinit {
  int mode;
  int cvstate;
  int forkmax;
  int* forkcurr;
  int shmid;
  std::terminate_handler _th;
  int __forked;

 public:
  __zinit(const char* IPC_CLEANUP_ENVAR = "__IPC_CLEANUP", const char* DEBUG_ENVAR = "__RUNDEBUG", 
          const char* RUNTIME_LIMIT_ENVAR = "__RUNTIMELIMIT", const char* FORKMAX_ENVAR = "__FORKMAX") 
          : forkmax(0), shmid(0), __forked(0) {
    // initialization
    mode = __ae_thread_swapmode(__AE_ASCII_MODE);
    cvstate = __ae_autoconvert_state(_CVTSTATE_QUERY);
    if (_CVTSTATE_OFF == cvstate) {
      __ae_autoconvert_state(_CVTSTATE_ON);
    }
    char* cu = __getenv_a(IPC_CLEANUP_ENVAR);
    if (cu && !memcmp(cu, "1", 2)) {
      cleanupipc(1);
    }
    char* dbg = __getenv_a(DEBUG_ENVAR);
    if (dbg && !memcmp(dbg, "1", 2)) {
      __debug_mode = 1;
    }
    char* tl = __getenv_a(RUNTIME_LIMIT_ENVAR);
    if (tl) {
      int sec = __atoi_a(tl);
      if (sec > 0) {
        __settimelimit(sec);
      }
    }
    char* fm = __getenv_a(FORKMAX_ENVAR);
    if (fm) {
      int v = __atoi_a(fm);
      if (v > 0) {
        forkmax = v;
        char path[1024];
        if (0 == getcwd(path, sizeof(path))) strcpy(path, "./");
        key_t key = ftok(path, 9021);
        shmid = shmget(key, 1024, 0666 | IPC_CREAT);
        forkcurr = (int*)shmat(shmid, (void*)0, 0);
        *forkcurr = 0;
      }
    }
    char* tenv = getenv("_EDC_SIG_DFLT");
    if (!tenv || !*tenv) {
      setenv("_EDC_SIG_DFLT","1",1);
    }
    _th = std::get_terminate();
    std::set_terminate(abort);
  }
  int forked(int newvalue) {
    int old = __forked;
    __forked = newvalue;
    return old;
  }
  int get_forkmax(void) { return forkmax; }
  int inc_forkcount(void) {
    if (0 == forkmax || 0 == shmid) return 0;
    int original;
    int new_value;

    do {
      original = *forkcurr;
      new_value = original + 1;
      __asm(" cs %0,%2,%1 \n "
            : "+r"(original), "+m"(*forkcurr)
            : "r"(new_value)
            :);
    } while (original != (new_value - 1));
    return new_value;
  }
  int dec_forkcount(void) {
    if (0 == forkmax || 0 == shmid) return 0;
    int original;
    int new_value;

    do {
      original = *forkcurr;
      if (original == 0) return 0;
      new_value = original - 1;
      __asm(" cs %0,%2,%1 \n "
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
      if (__forked) dec_forkcount();
      shmdt(forkcurr);
      shmctl(shmid, IPC_RMID, 0);
    }
    cleanupipc(0);
  }
  void __abort() { _th(); }
};

class __setlibpath {

public:
  __setlibpath() {
    std::vector<char> argv(512, 0);
    std::vector<char> parent(512, 0);
    W_PSPROC buf;
    int token = 0;
    pid_t mypid = getpid();
    memset(&buf, 0, sizeof(buf));
    buf.ps_pathlen = argv.size();
    buf.ps_pathptr = &argv[0];
    while ((token = w_getpsent(token, &buf, sizeof(buf))) > 0) {
      if (buf.ps_pid == mypid) {
        /* Found our process. */

        /* Resolve path to find true location of executable. */
        if (realpath(&argv[0], &parent[0]) == NULL)
          break;

        /* Get parent directory. */
        dirname(&parent[0]);

        /* Get parent's parent directory. */
        std::vector<char> parent2(parent.begin(), parent.end());
        dirname(&parent2[0]);

        /* Append new paths to libpath. */
        std::ostringstream libpath;
        libpath << getenv("LIBPATH");
        libpath << ":" << &parent[0] << "/lib.target/";
        libpath << ":" << &parent2[0] << "/lib/";
        setenv("LIBPATH", libpath.str().c_str(), 1);
        break;
      }
    }
  }
};


#endif

#endif
