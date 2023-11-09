///////////////////////////////////////////////////////////////////////////////
// Licensed Materials - Property of IBM
// ZOSLIB
// (C) Copyright IBM Corp. 2020. All Rights Reserved.
// US Government Users Restricted Rights - Use, duplication
// or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
///////////////////////////////////////////////////////////////////////////////

#include "zos-sys-info.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

// Byte 6 of CVTOSLVL
// (https://www.ibm.com/docs/en/zos/2.4.0?topic=information-cvt-mapping)
static const uint8_t __ZOSLVL_V2R1 = 0x80;
static const uint8_t __ZOSLVL_V2R2 = 0x40;
static const uint8_t __ZOSLVL_V1R13 = 0x20;
static const uint8_t __ZOSLVL_V2R3 = 0x10;
static const uint8_t __ZOSLVL_V2R4 = 0x08;
static const uint8_t __ZOSLVL_V2R5 = 0x04;

#ifdef __cplusplus
extern "C" {
#endif

int __get_num_online_cpus(void) {
  ZOSCVT *__ptr32 cvt = ((ZOSPSA *)0)->cvt;
  ZOSRMCT *__ptr32 rmct = cvt->rmct;
  ZOSCCT *__ptr32 cct = rmct->cct;
  return static_cast<int>(cct->cpuCount);
}

int __get_num_frames(void) {
  ZOSCVT *__ptr32 cvt = ((ZOSPSA *)0)->cvt;
  ZOSRCE *__ptr32 rce = cvt->rce;
  return static_cast<int>(rce->pool);
}

// Adapted from OMR codebase: https://github.com/eclipse/omr/blob/master/port/unix/omrsysinfo.c#L4629
int getloadavg(double loadavg[], int nelem) {
  if (nelem > 3 || nelem <= 0)
    return -1;
  
  ZOSCVT* __ptr32 cvt = ((ZOSPSA*)0)->cvt;
  ZOSRMCT* __ptr32 rcmt = cvt->rmct;
  ZOSCCT* __ptr32 cct = rcmt->cct;

  //Hack: z/OS does not get cpu load in samples, just use the same value
  for (int i = 0; i < nelem; i++) 
    loadavg[i] = (double)cct->ccvutilp / 100.0;

  return nelem;
}

oslvl_t __get_os_level(void) {
  static oslvl_t oslvl = ZOSLVL_UNKNOWN;
  if (oslvl != ZOSLVL_UNKNOWN)
    return oslvl;
  ZOSCVT *__ptr32 cvt = ((ZOSPSA *)0)->cvt;
  uint8_t lvl = cvt->cvtoslvl[6];

  // Note: lvl has to be checked in this order, because the bits in byte 6 are
  // set for the current level and all those before it. There is also the
  // exception for V1R13, which if its bit is set, also has the bits for V2R2
  // and V2R1 set.
  if (lvl & __ZOSLVL_V2R5)
    return (oslvl = ZOSLVL_V2R5);
  if (lvl & __ZOSLVL_V2R4)
    return (oslvl = ZOSLVL_V2R4);
  if (lvl & __ZOSLVL_V2R3)
    return (oslvl = ZOSLVL_V2R3);
  if (lvl & __ZOSLVL_V2R2)
    return (oslvl = ZOSLVL_V2R2);
  if (lvl & __ZOSLVL_V2R1)
    return (oslvl = ZOSLVL_V2R1);
  if (lvl & __ZOSLVL_V1R13)
    return (oslvl = ZOSLVL_V1R13);
  fprintf(stderr, "Unknown OS level %x\n", lvl);
  assert(0);
  return ZOSLVL_UNKNOWN; // so compiler doesn't complain
}

bool __is_os_level_at_or_above(oslvl_t level) {
  return (__get_os_level() >= level);
}

bool __is_stfle_available() {
  // PSA decimal offset 200 from address 0 is Facilities List:
  // https://www.ibm.com/docs/en/zos/2.4.0?topic=information-psa-mapping
  // and bit 7 (0x01) specifies if STFLE instruction is available in:
  // https://www.ibm.com/docs/en/zos/2.4.0?topic=information-ihafacl-mapping
  uint8_t FACLBYTE0 = *(reinterpret_cast<uint8_t *>(200));
  return ((FACLBYTE0 & 0x01) != 0);
}

bool __is_vxf_available() {
  // vxf is Vector Extension Facility; from CVT:
  // https://www.ibm.com/docs/en/zos/2.4.0?topic=correlator-cvt-information
  ZOSCVT *__ptr32 cvt = ((ZOSPSA *)0)->cvt;
  uint8_t CVTFLAG5 = (reinterpret_cast<uint8_t *>(cvt))[244];
  return ((CVTFLAG5 & 0x80) != 0);
}

bool __is_vef1_available() {
  // According to
  // https://www.ibm.com/docs/en/zos/2.4.0?topic=information-ihafacl-mapping:
  // "Even if this bit (FACL_VEF1) is on, do not use the VEF1 unless bit CVTVEF
  // is on" - so check that first:
  if (!__is_vxf_available())
    return false;
  // PSA decimal offset 216 from address 0 is Facilities List bytes 16-31:
  // https://www.ibm.com/docs/en/zos/2.4.0?topic=information-psa-mapping
  // and bit x'01' in IHAFACL determines Vector Enhancements Facility 1:
  // https://www.ibm.com/docs/en/zos/2.4.0?topic=information-ihafacl-mapping
  uint8_t FACLBYTE16 = *(reinterpret_cast<uint8_t *>(216));
  return ((FACLBYTE16 & 0x01) != 0);
}

char *__get_cpu_model(char *buffer, size_t size) {
  ZOSCVT *__ptr32 cvt = ((ZOSPSA *)0)->cvt;
  ZOSPCCAVT *__ptr32 pccavt = cvt->pccavt;
  ZOSPCCA *__ptr32 cpu = pccavt->cpu0;

  size_t n = size > ZOSCPU_MODEL_LENGTH ? ZOSCPU_MODEL_LENGTH : size - 1;

  memcpy(buffer, cpu->cpu_model, n);
  buffer[n] = '\0';
  __e2a_l(buffer, n);

  return buffer;
}

#ifdef __cplusplus
}
#endif
