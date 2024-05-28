#define _AE_BIMODAL 1
#ifdef __ibmxl__
#undef _ENHANCED_ASCII_EXT
#define _ENHANCED_ASCII_EXT 0xFFFFFFFF
#endif
#define _XOPEN_SOURCE 600
#define _OPEN_SYS_FILE_EXT 1
#define _OPEN_MSGQ_EXT 1

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <unistd.h>

#include "zos-base.h"
#include "zos-getentropy.h"

#if ' ' != 0x20
#error not build with correct codeset
#endif

static unsigned char _value(int bit) {
  unsigned long long t0, t1 = 0, start;
  int i;
  __asm volatile(" la 15,0 \n svc 137\n" ::: "r15", "r6");
  (void) __stckf(&start);
  start = start >> bit;
  for (i = 0; i < 400; ++i) {
    __asm volatile(" la 15,0 \n svc 137\n" ::: "r15", "r6");
    (void) __stckf(&t0);
    t0 = t0 >> bit;
    if ((t0 - start) > 0xfffff) {
      break;
    }
    t1 ^= t0;
  }
  return (unsigned char)t1;
}

static void _slow(int size, void* output) {
  char* out = (char*)output;
  int i;
#ifdef _LP64
  static int bits = 0;
#else
  int bits = 0;
#endif
  unsigned long long t0, t1, r, m = 0xffff;
  unsigned int zbitcnt[] = {0xffffffff, 0,  1,  26, 2,  23, 27, 0, 3,  16,
                            24,         30, 28, 11, 0,  13, 4,  7, 17, 0,
                            25,         22, 31, 15, 29, 19, 12, 6, 0,  21,
                            14,         9,  5,  20, 8,  19, 18};
  unsigned int t = 0xaa;

  while (bits == 0 || bits > 11) {
    for (i = 0; i < 10; ++i) {
      (void) __stckf(&t0);
      (void) __stckf(&t1);
      r = t0 ^ t1;
      if (r < m) m = r;
    }
    bits = zbitcnt[(-m & m) % 37];
  }
  for (i = 0; i < size; ++i) {
    t ^= _value(bits);
    out[i] = t;
  }
}

#if defined(ZOSLIB_ENABLE_V2R5_FEATURES)
extern "C" int __getentropy(void* output, size_t size) {
#else
extern "C" int getentropy(void* output, size_t size) {
#endif
  if (size > 257) {
    errno = EIO;
    return -1;
  }
  char* out = (char*)output;
#ifdef _LP64
  static int feature = -1;
#else
  int feature = -1;
#endif

  typedef struct parm {
    unsigned long long a;
    unsigned long long b;
  } parm_t;

  if (feature == -1) {
    if (0x40 & *(char*)(207)) {
      volatile parm_t value = {0, 0};
      __asm volatile(" prno 8,10\n"
            " jo *-4\n"
            :
            : __ZL_NR("",r0)(0), __ZL_NR("",r1)(&value)
            :);
    if (0x2000 & value.b) {
        feature = 1;
        // parm bit 114 is on
      } else {
        feature = 0;
      }
    } else {
      feature = 0;
    }
  }

  if (feature == 0) {
    _slow(size, out);
    return 0;
  }

#ifdef __XPLINK__
  // FIXME:
  // PRNO states that in 64-bit amode, all the 64-bits
  // of the even-odd register pair will be used.
  // The problem is "NR:r11"(0), generates a lhi 11,0 insn
  // This causes problems especially if the top half of lhi
  // isn't 0. So for now, declare it as a long, to
  // force the "lghi" instruction to be generated.
  // This will ensure the top half of the register will be
  // cleared.
  long first_operand_length = 0;
  __asm volatile(" prno 10,2\n"
        " jo *-4\n"
        : __ZL_NR("+",r2)(out), __ZL_NR("+",r3)(size)
        : __ZL_NR("",r0)(114), __ZL_NR("",r11)(first_operand_length)
        :);

#else
  __asm(" prno 8,10\n"
      " jo *-4\n"
      : __ZL_NR("+",r10)(out), __ZL_NR("+",r11)(size)
      : __ZL_NR("",r0)(114), __ZL_NR("",r9)(0)
      : "r0");
#endif
  return 0;
}
