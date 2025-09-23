///////////////////////////////////////////////////////////////////////////////
// Licensed Materials - Property of IBM
// ZOSLIB
// (C) Copyright IBM Corp. 2020. All Rights Reserved.
// US Government Users Restricted Rights - Use, duplication
// or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
///////////////////////////////////////////////////////////////////////////////

#define _AE_BIMODAL 1
#include "edcwccwi.h"
#include "zos.h"

#include "zos-macros.h"
#include <_Ccsid.h>
#include <_Nascii.h>
#include <__le_api.h>
#include <assert.h>
#include <builtins.h>
#include <ctest.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <iconv.h>
#include <libgen.h>
#include <map>
#include <mutex>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/msg.h>
#include <sys/ps.h>
#include <sys/shm.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

// Instrumentation code - for profiling
// when an application is built with zoslib, along with the
// -finstrument-functions option it will generate a json file in the cwd (set
// ZOSLIB_PROF_PATH to override), which can be analyzed using chrome tracing or
// perfetto (https://ui.perfetto.dev/)
namespace {
FILE *__prof_json_file = NULL;
pthread_mutex_t __prof_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t __prof_mutex2 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t __prof_mutex3 = PTHREAD_MUTEX_INITIALIZER;
char __profiling_file[PATH_MAX] = {0};
int __prof_isProfiling = 0;
int __prof_isDisabled = 0;
uint64_t __total_allocated_memory = 0;
std::map<void *, size_t> allocation_map;
} // namespace

extern "C" {

__attribute__((no_instrument_function)) static inline uint64_t get_timens() {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_nsec;
}

__attribute__((no_instrument_function)) static void check_env_vars() {
  char *disable_prof = getenv("ZOSLIB_PROF_DISABLE");
  if (disable_prof != NULL) {
    __prof_isDisabled = 1;
    return;
  }

  char *prof_path = getenv("ZOSLIB_PROF_PATH");
  if (prof_path != NULL) {
    sprintf(__profiling_file, "%s", prof_path);
  } else {
    sprintf(__profiling_file, "%s-%lu.json", getprogname(), get_timens());
  }
}

// Using the chrome trace event format:
// https://docs.google.com/document/d/1CvAClvFfyA5R-PhYUmn5OOQtYMH4h6I0nSsKchNAySU/preview
__attribute__((no_instrument_function))
static void __write_json_object(const char *name, const char *phase,
                    const char *cat = "PERF", int64_t delta_memory = 0) {
  pthread_mutex_lock(&__prof_mutex);
  if (delta_memory != 0) {
    __total_allocated_memory += delta_memory;
    fprintf(__prof_json_file,
            "  {\"cat\": \"%s\", \"name\": \"%s\", \"ph\": \"%s\", \"pid\": "
            "%d, \"tid\": %d, \"ts\": %lu, \"args\": {\"delta_memory\": %lld, "
            "\"total_memory\": %zu}},\n",
            cat, name, phase, getpid(), gettid(), get_timens(), delta_memory,
            __total_allocated_memory);
  } else {
    fprintf(__prof_json_file,
            "  {\"cat\": \"%s\", \"name\": \"%s\", \"ph\": \"%s\", \"pid\": "
            "%d, \"tid\": %d, \"ts\": %lu},\n",
            cat, name, phase, getpid(), gettid(), get_timens());
  }
  pthread_mutex_unlock(&__prof_mutex);
}

__attribute__((no_instrument_function)) __attribute__((destructor)) 
void close_json_file() {
  if (__prof_isProfiling)
    if (__prof_json_file != NULL) {
      fprintf(__prof_json_file, "null ]}\n");
      fclose(__prof_json_file);

      FILE *fp = popen("command -v gzip", "r");
      if (fp != NULL) {
        char path[PATH_MAX];
        if (fgets(path, sizeof(path), fp) != NULL) {
          char command[PATH_MAX];
          snprintf(command, sizeof(command), "gzip -f %s 2>/dev/null",
                   __profiling_file);
          system(command); // Should we handle this if it fails?
        }
        pclose(fp);
      }
    }
}

__attribute__((no_instrument_function)) __Z_EXPORT void
__cyg_profile_func_enter(void *this_fn, void *call_site) {
  if (__prof_isDisabled)
    return;

  // On first call, check envars
  if (!__prof_isProfiling) {
    __prof_isProfiling = 1;
    check_env_vars();
    if (__prof_isDisabled)
      return;

    __prof_json_file = fopen(__profiling_file, "w");
    if (__prof_json_file == NULL) {
      fprintf(stderr,
              "Error opening file profiling file %s for write, errno: %d",
              __profiling_file, errno);
      exit(1);
    }
    fprintf(__prof_json_file, "{ \"traceEvents\": [\n");
  }

  int prevError = errno;
  __stack_info si;
  void *cur_dsa = __dsa();

  if (__iterate_stack_and_get(cur_dsa, &si) != 0)
    __write_json_object(si.entry_name, "B");
  errno = prevError;
}

__attribute__((no_instrument_function)) __Z_EXPORT void
__cyg_profile_func_exit(void *this_fn, void *call_site) {
  if (__prof_isDisabled)
    return;

  int prevError = errno;
  __stack_info si;
  void *cur_dsa = __dsa();

  if (__iterate_stack_and_get(cur_dsa, &si) != 0)
    __write_json_object(si.entry_name, "E");
  errno = prevError;
}

void *__real_malloc(size_t size) asm("malloc");
void __real_free(void *ptr) asm("free");

// Custom malloc with profiling
__attribute__((no_instrument_function)) void *__zoslib_malloc(size_t size) {
  if (!__prof_isProfiling || __prof_isDisabled) {
    return __real_malloc(size);
  }

  void *ptr = __real_malloc(size);
  int old_errno = errno;
  if (ptr != NULL) {
    pthread_mutex_lock(&__prof_mutex2);
    allocation_map[ptr] = size;
    pthread_mutex_unlock(&__prof_mutex2);
    __write_json_object("Memory", "C", "malloc", size);
  }

  errno = old_errno;
  return ptr;
}

__attribute__((no_instrument_function))
size_t malloc_usable_size(void *ptr) {
  if (ptr == NULL) {
    return 0;
  }

  auto it = allocation_map.find(ptr);
  if (it != allocation_map.end()) {
    return it->second;
  }

  return 0;
}

__attribute__((no_instrument_function)) void __zoslib_free(void *ptr) {
  if (!__prof_isProfiling || __prof_isDisabled) {
    __real_free(ptr);
    return;
  }

  int prevError = errno;
  if (ptr != NULL) {
    int64_t size = malloc_usable_size(ptr); 

    pthread_mutex_lock(&__prof_mutex2);
    allocation_map.erase(ptr);
    pthread_mutex_unlock(&__prof_mutex2);

    __write_json_object("Memory", "C", "free", -size);
  }

  errno = prevError;
  __real_free(ptr);
}

} // extern "C"
