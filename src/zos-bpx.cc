///////////////////////////////////////////////////////////////////////////////
// Licensed Materials - Property of IBM
// ZOSLIB
// (C) Copyright IBM Corp. 2020. All Rights Reserved.
// US Government Users Restricted Rights - Use, duplication
// or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
///////////////////////////////////////////////////////////////////////////////

#include "zos-bpx.h"
#include "zos-base.h"

#define _POSIX_SOURCE
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
//SDP#include <mutex>

#ifdef __cplusplus
extern "C" {
#endif
//
// Call setup information:
// https://www.ibm.com/support/knowledgecenter/SSLTBW_2.3.0/com.ibm.zos.v2r3.bpxb100/bpx2cr_Example.htm
//
// List of offsets for USS APIs:
// https://www.ibm.com/support/knowledgecenter/SSLTBW_2.3.0/com.ibm.zos.v2r3.bpxb100/bpx2cr_List_of_offsets.htm
//
char *__ptr32 *__ptr32 __uss_base_address(void) {
  static char *__ptr32 *__ptr32 res = 0;
  if (res == 0) {
    res = ((char *__ptr32 *__ptr32 *__ptr32 *__ptr32 *)0)[4][136][6];
  }
  return res;
}

static ssize_t pathmax_size(const char* path) {
  ssize_t pathmax;

  pathmax = pathconf(path, _PC_PATH_MAX);

  if (pathmax == -1)
    pathmax = PATH_MAX;

  return pathmax;
}

#if TRACE_ON // for debugging use

char *__realpath(const char *path, char *resolved_path) asm("@@A00187");
char *__realpath(const char *path, char *resolved_path) {
   void *reg15 = __uss_base_address()[884 / 4]; // BPX4RPH offset is 884
   int rv, rc, rn, path_len, path_resolved_len;
   char *ebcdic_path = (char*) malloc(strlen(path) + 1);
   strcpy(ebcdic_path, path);
   __a2e_s(ebcdic_path);
   path_len = strlen(path);
   if (resolved_path == NULL) {
      resolved_path = (char*) malloc(pathmax_size(path) + 1);
   }
   path_resolved_len=strlen(resolved_path);
   const void *argv[] = {&path_len, ebcdic_path, &path_resolved_len, resolved_path, &rv, &rc, &rn};
   __asm volatile(" basr 14,%0\n"
                  : "+NR:r15"(reg15)
                  : "NR:r1"(&argv)
                  : "r0", "r14");
   free(ebcdic_path);
   if (-1 == rv) {
     __console_printf("%s:%s:%d path: %s rval: %d rcode: %d reason: %d\n",
                       __FILE__, __FUNCTION__, __LINE__, path, rv, rc, rn);
     errno = rc;
     return NULL;
   }
   __e2a_s(resolved_path);
   __console_printf("%s:%s:%d path: %s pathresolved: %s length: %d\n",
                     __FILE__, __FUNCTION__, __LINE__, path, resolved_path, rv);
   return resolved_path;
}
#endif // if TRACE_ON - for debugging use

// C Library Overrides
//-----------------------------------------------------------------

char *__realpath_orig(const char __restrict__ *path, char __restrict__ *resolved_path) asm("@@A00187");

char *__realpath_extended(const char __restrict__ *path, char __restrict__ *resolved_path) {
   if (resolved_path == NULL) {
      resolved_path = (char*) malloc(pathmax_size(path) + 1);
   }
   return __realpath_orig(path, resolved_path);
}


void __bpx4kil(int pid, int signal, void *signal_options, int *return_value,
               int *return_code, int *reason_code) {
  void *reg15 = __uss_base_address()[308 / 4]; // BPX4KIL offset is 308
  void *argv[] = {&pid,         &signal,     signal_options,
                  return_value, return_code, reason_code}; // os style parm list
  __asm volatile(" basr 14,%0\n"
                 : "+NR:r15"(reg15)
                 : "NR:r1"(&argv)
                 : "r0", "r14");
}

void __bpx4frk(int *pid, int *return_code, int *reason_code) {
  void *reg15 = __uss_base_address()[240 / 4];    // BPX4FRK offset is 240
  void *argv[] = {pid, return_code, reason_code}; // os style parm list
  __asm volatile(" basr 14,%0\n"
                 : "+NR:r15"(reg15)
                 : "NR:r1"(&argv)
                 : "r0", "r14");
}

void __bpx4ctw(unsigned int *secs, unsigned int *nsecs,
               unsigned int *event_list, unsigned int *secs_rem,
               unsigned int *nsecs_rem, int *return_value, int *return_code,
               int *reason_code) {
  void *reg15 = __uss_base_address()[492 / 4]; // BPX4CTW offset is 492
  void *argv[] = {secs,         nsecs,       event_list, secs_rem, nsecs_rem,
                  return_value, return_code, reason_code}; // os style parm list
  __asm volatile(" basr 14,%0\n"
                 : "+NR:r15"(reg15)
                 : "NR:r1"(&argv)
                 : "r0", "r14");
}

void __bpx4gth(int *input_length, void **input_address, int *output_length,
               void **output_address, int *return_value, int *return_code,
               int *reason_code) {
  // BPX4GTH (__getthent) offset is 1056, as specified in:
  // https://www.ibm.com/docs/en/zos/2.4.0?topic=scocs-list-offsets
  void *reg15 = __uss_base_address()[1056 / 4];

  void *argv[] = {input_length, input_address, output_length, output_address,
                  return_value, return_code,   reason_code};

  __asm volatile(" basr 14,%0\n"
                 : "+NR:r15"(reg15)
                 : "NR:r1"(&argv)
                 : "r0");
}

void __bpx4lcr(int pathname_length, char *pathname, int attributes_length,
               __bpxyatt_t *attributes, int *return_value, int *return_code,
               int *reason_code) {
  // BPX4LCR offset is 1180.
  void *reg15 = __uss_base_address()[1180 / 4];

  // os style parm list.
  void *argv[] = {&pathname_length, pathname,    &attributes_length, attributes,
                  return_value,     return_code, reason_code};

  __asm volatile(" basr 14,%0\n"
                 : "+NR:r15"(reg15)
                 : "NR:r1"(&argv)
                 : "r0", "r14");
}

#ifdef __cplusplus
}
#endif
