///////////////////////////////////////////////////////////////////////////////
// Licensed Materials - Property of IBM
// ZOSLIB
// (C) Copyright IBM Corp. 2023. All Rights Reserved.
// US Government Users Restricted Rights - Use, duplication
// or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
///////////////////////////////////////////////////////////////////////////////

#ifndef ZOS_SYS_STDLIB_H_
#define ZOS_SYS_STDLIB_H_

#include <sys/types.h>
#ifdef ZOSLIB_OVERRIDE_CLIB_ALLOCS
#define __XPLAT 1
#include "zos-macros.h"
#include "zos-io.h"

#if defined(__cplusplus)
extern "C" {
#endif

void *__calloc_orig(size_t, size_t)        asm("calloc");
void *__malloc_orig(size_t)                asm("malloc");
void *__malloc31_orig(size_t)              asm("__malloc31");
void *__realloc_orig(void *, size_t)       asm("realloc");
void __free_orig(void *)                   asm("free");
void __free31_orig(void *addr, size_t len) asm("free");

__Z_EXPORT void * __calloc_trace(size_t, size_t);
__Z_EXPORT void * __malloc_trace(size_t);
__Z_EXPORT void * __malloc31_trace(size_t);
__Z_EXPORT void * __realloc_trace(void *, size_t);
__Z_EXPORT void   __free_trace(void *);
__Z_EXPORT void   __free31_trace(void *, size_t);

#if defined(__cplusplus)
};
#endif

#undef calloc
#undef free
#undef malloc
#undef __malloc31
#undef __free31
#undef realloc

#ifdef ZOSLIB_OVERRIDE_CLIB_ALLOCS_PRTSRC

#include <libgen.h> // for basename()

#define __calloc_prtsrc(x, y) \
  ({ \
    if (__doLogMemoryAll()) { \
      __memprintf("CALLOC %s:%d\n", basename(__FILE__),__LINE__); \
    } \
    void *p = __calloc_trace(x, y); \
    p; \
  })
#define __malloc_prtsrc(x) \
  ({ \
    if (__doLogMemoryAll()) { \
      __memprintf("MALLOC %s:%d\n", basename(__FILE__),__LINE__); \
    } \
    void *p = __malloc_trace(x); \
    p; \
  })
#define __malloc31_prtsrc(x) \
  ({ \
    if (__doLogMemoryAll()) { \
      __memprintf("MALLOC-31 %s:%d\n", basename(__FILE__),__LINE__); \
    } \
    void *p = __malloc31_trace(x); \
    p; \
  })
#define __realloc_prtsrc(x, y) \
  ({ \
    if (__doLogMemoryAll()) { \
      __memprintf("REALLOC %s:%d\n", basename(__FILE__),__LINE__); \
    } \
    void *p = __realloc_trace(x, y); \
    p; \
  })
#define __free_prtsrc(x) ({ \
    if (__doLogMemoryAll()) { \
      __memprintf("FREE %s:%d\n", basename(__FILE__),__LINE__); \
    } \
    __free_trace(x); \
  })
#define __free31_prtsrc(x,y) ({ \
    if (__doLogMemoryAll()) { \
      __memprintf("FREE-31 %s:%d\n", basename(__FILE__),__LINE__); \
    } \
    __free31_trace(x,y); \
  })

#endif // ZOSLIB_OVERRIDE_CLIB_ALLOCS_PRTSRC

// This is so they can be mapped after include_next <stdlib.h>:
#define calloc     __calloc_replaced
#define malloc     __malloc_replaced
#define __malloc31 __malloc31_replaced
#define realloc    __realloc_replaced
#define free       __free_replaced

#include_next <stdlib.h>

#undef calloc
#undef free
#undef malloc
#undef __malloc31
#undef realloc

#ifdef ZOSLIB_OVERRIDE_CLIB_ALLOCS_PRTSRC
#define calloc     __calloc_prtsrc
#define malloc     __malloc_prtsrc
#define realloc    __realloc_prtsrc
#define __malloc31 __malloc31_prtsrc
#define __free31   __free31_prtsrc
// This would cause failure:
// .../include/c++/v1/locale:283:68: error: reference to unresolved using declaration
// unique_ptr<unsigned char, void(*)(void*)> __stat_hold(nullptr, free);
// #define free       __free_prtsrc
void free(void *) asm("__free_trace");

#else

void *calloc(size_t, size_t)  asm("__calloc_trace");
void *malloc(size_t)          asm("__malloc_trace");
void *realloc(void *, size_t) asm("__realloc_trace");
void free(void *)             asm("__free_trace");
void *__malloc31(size_t)      asm("__malloc31_trace");
void __free31(void *, size_t) asm("__free31_trace");

#endif // ZOSLIB_OVERRIDE_CLIB_ALLOCS_PRTSRC

#else
#include_next <stdlib.h>

#define __free31(x,y) free(x)

#endif  // ZOSLIB_OVERRIDE_CLIB_ALLOCS
#endif  // ZOS_SYS_STDLIB_H_
