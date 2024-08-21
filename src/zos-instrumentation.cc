///////////////////////////////////////////////////////////////////////////////
// Licensed Materials - Property of IBM
// ZOSLIB
// (C) Copyright IBM Corp. 2020. All Rights Reserved.
// US Government Users Restricted Rights - Use, duplication
// or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
///////////////////////////////////////////////////////////////////////////////

#define _AE_BIMODAL 1
#include "zos.h"
#include "edcwccwi.h"

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
#include "zos-macros.h"

// Instrumentation code - for profiling
// when an application is built with zoslib, along with the -finstrument-functions option
// it will generate a json file in the cwd (set ZOSLIB_PROF_PATH to override), which can be 
// analyzed using chrome tracing or perfetto (https://ui.perfetto.dev/)
namespace {
FILE* __prof_json_file = NULL;
pthread_mutex_t __prof_mutex = PTHREAD_MUTEX_INITIALIZER;
char __profiling_file[PATH_MAX] = {0};
int __prof_isProfiling = 0;
int __prof_isDisabled = 0;
}

extern "C" {

__attribute__((no_instrument_function))
static inline uint64_t get_timens() {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_nsec;
}

__attribute__((no_instrument_function))
static void check_env_vars() {
  char* disable_prof = getenv("ZOSLIB_PROF_DISABLE");
  if (disable_prof != NULL) {
    __prof_isDisabled = 1;
    return;
  }

  char* prof_path = getenv("ZOSLIB_PROF_PATH");
  if (prof_path != NULL) {
    sprintf(__profiling_file, "%s", prof_path);
  } else {
    sprintf(__profiling_file, "%s-%lu.json", getprogname(), get_timens());
  }
}

// Using the chrome trace event format: https://docs.google.com/document/d/1CvAClvFfyA5R-PhYUmn5OOQtYMH4h6I0nSsKchNAySU/preview
__attribute__((no_instrument_function))
static void write_json_object(const char* name, const char* phase) {
  pthread_mutex_lock(&__prof_mutex);
  fprintf(__prof_json_file, "  {\"cat\": \"PERF\", \"name\": \"%s\", \"ph\": \"%s\", \"pid\": %d, \"tid\": %d, \"ts\": %lu},\n", name, phase, getpid(), gettid(), get_timens());
  pthread_mutex_unlock(&__prof_mutex);
}

__attribute__((no_instrument_function))
__attribute__((destructor))
void close_json_file() {
  if (__prof_isProfiling)
    if (__prof_json_file != NULL) {
      fprintf(__prof_json_file, "null ]}\n");
      fclose(__prof_json_file);
    }
}

__attribute__((no_instrument_function))
__Z_EXPORT void __cyg_profile_func_enter(void* this_fn, void* call_site) {
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
      fprintf(stderr, "Error opening file profiling file %s for write, errno: %d", __profiling_file, errno);
      exit(1);
    }
    fprintf(__prof_json_file, "{ \"traceEvents\": [\n");
  }

  __stack_info si;
  void *cur_dsa = __dsa();

  __iterate_stack_and_get(cur_dsa, &si);
  write_json_object(si.entry_name, "B");
}

__attribute__((no_instrument_function))
__Z_EXPORT void __cyg_profile_func_exit(void* this_fn, void* call_site) {
  if (__prof_isDisabled)
    return;

  __stack_info si;
  void *cur_dsa = __dsa();

  __iterate_stack_and_get(cur_dsa, &si);
  write_json_object(si.entry_name, "E");
}

} 
