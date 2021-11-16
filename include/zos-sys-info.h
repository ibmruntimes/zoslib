///////////////////////////////////////////////////////////////////////////////
// Licensed Materials - Property of IBM
// ZOSLIB
// (C) Copyright IBM Corp. 2020. All Rights Reserved.
// US Government Users Restricted Rights - Use, duplication
// or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
///////////////////////////////////////////////////////////////////////////////

// APIs that obtain various z/OS system info.

#ifndef ZOS_SYS_INFO_H_
#define ZOS_SYS_INFO_H_

#include <stdint.h>

typedef enum {
  ZOSLVL_V1R13 = 0,
  ZOSLVL_V2R1,
  ZOSLVL_V2R2,
  ZOSLVL_V2R3,
  ZOSLVL_V2R4,
  ZOSLVL_V2R5,
  ZOSLVL_UNKNOWN,
} oslvl_t;

// CPU Management Control Table (CCT).
typedef struct ZOSCCT {
  uint8_t filler[110];
  uint16_t cpuCount; // Number of online CPUs.
} ZOSCCT_t;

// System Resources Manager Control Table (RMCT).
typedef struct ZOSRMCT {
  uint8_t name[4];
  struct ZOSCCT *__ptr32 cct;
} ZOSRMCT_t;

// RSM Control and Enumeration Area (RCE).
typedef struct ZOSRCE {
  uint8_t id[4];
  uint32_t pool; // Number of frames currently available to system.
} ZOSRCE_t;

// Communications Vector Table (CVT).
typedef struct ZOSCVT {
  uint8_t filler[604];
  struct ZOSRMCT *__ptr32 rmct;
  uint8_t filler1[560];
  struct ZOSRCE *__ptr32 rce;
  uint8_t filler2[92];
  uint8_t cvtoslvl[16];
} ZOSCVT_t;

// Prefixed Save Area (PSA).
// Maps storage starting at location 0.
typedef struct ZOSPSA {
  uint8_t filler[16]; // Ignore 16 bytes before CVT pointer.
  struct ZOSCVT *__ptr32 cvt;
} ZOSPSA_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Get the number of online CPUs
 * \return returns the number of online CPUs
 */
int __get_num_online_cpus(void);

/**
 * Get the number of frames currently available to the system
 * \return returns the number of available frames
 */
int __get_num_frames(void);

/**
 * Get the OS level
 * \return the OS level as ZOSLVL_V2R1/2/3/4/5 (values are in ascending order)
 */
oslvl_t __get_os_level(void);

/**
 * Check if current OS is at or above a given level
 * \return true if the current OS level is at or above the given level, and false otherwise
 */
bool __is_os_level_at_or_above(oslvl_t level);

/**
 * Check if the current z arch includes Vector Extension Facility
 * \return true if Vector Extension Facility instructions are available, and false otherwise
 */
bool __is_vxf_available();

/**
 * Check if the current z arch includes Vector Enhancements Facility 1
 * \return true if Vector Enhancements Facility 1 instructions are available, and false otherwise
 */
bool __is_vef1_available();

#ifdef __cplusplus
}
#endif
#endif // ZOS_SYS_INFO_H_
