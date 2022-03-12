///////////////////////////////////////////////////////////////////////////////
// Licensed Materials - Property of IBM
// ZOSLIB
// (C) Copyright IBM Corp. 2020. All Rights Reserved.
// US Government Users Restricted Rights - Use, duplication
// or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
///////////////////////////////////////////////////////////////////////////////

#include "zos-bpx.h"

#define _POSIX_SOURCE
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <mutex>

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
