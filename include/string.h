///////////////////////////////////////////////////////////////////////////////
// Licensed Materials - Property of IBM
// ZOSLIB
// (C) Copyright IBM Corp. 2023. All Rights Reserved.
// US Government Users Restricted Rights - Use, duplication
// or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
///////////////////////////////////////////////////////////////////////////////

#ifndef ZOS_SYS_STRING_H_
#define ZOS_SYS_STRING_H_

#include <sys/types.h>

#ifdef ZOSLIB_OVERRIDE_CLIB_ALLOCS
#include "zos-macros.h"
#include "zos-io.h"
#define __XPLAT 1

#if defined(__cplusplus)
extern "C" {
#endif

char *__strdup_orig(const char *) asm("strdup");

__Z_EXPORT char *__strdup_trace(const char *);

#if defined(__cplusplus)
};
#endif

#undef strdup

#ifdef ZOSLIB_OVERRIDE_CLIB_ALLOCS_PRTSRC
#include <libgen.h> // for basename()

#define __strdup_prtsrc(x) \
  ({ \
    if (__doLogMemoryAll()) { \
      __memprintf("STRDUP %s:%d\n",basename(__FILE__),__LINE__); \
    } \
    char *p = __strdup_trace(x); \
    p; \
  })
#endif // ZOSLIB_OVERRIDE_CLIB_ALLOCS_PRTSRC

#define strdup __strdup_replaced

#include_next <string.h>

#undef strdup

#ifdef ZOSLIB_OVERRIDE_CLIB_ALLOCS_PRTSRC
#define strdup __strdup_prtsrc
#else
char *strdup(const char *) asm("__strdup_trace");
#endif

#else
#include_next <string.h>

#define __strdup_orig strdup

#endif  // ZOSLIB_OVERRIDE_CLIB_ALLOCS
#endif  // ZOS_SYS_STRING_H_
