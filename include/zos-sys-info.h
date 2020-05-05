#ifndef ZOS_SYS_INFO_H_
#define ZOS_SYS_INFO_H_

// CPU Management Control Table (CCT).
typedef struct ZOSCCT {
  uint8_t filler [110];
  uint16_t cpuCount;  // Number of online CPUs.
} ZOSCCT_t;


// System Resources Manager Control Table (RMCT).
typedef struct ZOSRMCT {
  uint8_t name[4];
  struct ZOSCCT* __ptr32 cct;
} ZOSRMCT_t;


// RSM Control and Enumeration Area (RCE).
typedef struct ZOSRCE {
  uint8_t id[4];
  uint32_t pool;  // Number of frames currently available to system.
} ZOSRCE_t;


// Communications Vector Table (CVT).
typedef struct ZOSCVT {
  uint8_t filler[604];
  struct ZOSRMCT* __ptr32 rmct;
  uint8_t filler1[560];
  struct ZOSRCE* __ptr32 rce;
  uint8_t filler2[92];
  uint8_t cvtoslvl[16];
} ZOSCVT_t;


// Prefixed Save Area (PSA).
// Maps storage starting at location 0.
typedef struct ZOSPSA {
  uint8_t filler[16];  // Ignore 16 bytes before CVT pointer.
  struct ZOSCVT* __ptr32 cvt;
} ZOSPSA_t;

#endif  // ZOS_SYS_INFO_H_
