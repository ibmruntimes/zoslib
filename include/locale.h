///////////////////////////////////////////////////////////////////////////////
// Licensed Materials - Property of IBM
// ZOSLIB
// (C) Copyright IBM Corp. 2024. All Rights Reserved.
// US Government Users Restricted Rights - Use, duplication
// or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
///////////////////////////////////////////////////////////////////////////////

#ifndef ZOS_LOCALE_H_
#define ZOS_LOCALE_H_

// With the xlclang++ compiler, the libc++ it provides has definitions
// for these functions in the global space that conflict with these.
// For xlclang++, exclude these declarations unless ZOSLIB_OVERRIDE_CLIB_LOCALE
// is defined even if ZOSLIB_OVERRIDE_CLIB is defined.

#if !defined(__ibmxl__) && (defined(ZOSLIB_OVERRIDE_CLIB) || defined(ZOSLIB_OVERRIDE_CLIB_LOCALE)) && !defined(ZOSLIB_USE_CLIB_LOCALE)
#include "zos-macros.h"

#undef newlocale
#define newlocale_replaced
#undef freelocale
#define freelocale_replaced
#undef uselocale
#define uselocale_replaced
#include_next <locale.h>
#undef newlocale
#undef freelocale
#undef uselocale

#if defined(__cplusplus)
extern "C" {
#endif
# define _CATMASK(n)       (1 << (n))
# define LC_COLLATE_MASK   _CATMASK(LC_COLLATE)
# define LC_CTYPE_MASK     _CATMASK(LC_CTYPE)
# define LC_MONETARY_MASK  _CATMASK(LC_MONETARY)
# define LC_NUMERIC_MASK   _CATMASK(LC_NUMERIC)
# define LC_TIME_MASK      _CATMASK(LC_TIME)
# define LC_MESSAGES_MASK  _CATMASK(LC_MESSAGES)

// LC_ALL_MASK is an extension, and not defined in the standard.
// A little care is needed here.  The values assigned to LC_COLLATE etc.
// are not necessarily continuous.  Therefore, LC_ALL_MASK must be
// constructed by expicitly "or"ing the values of the individual categories.
# define LC_ALL_MASK       (LC_COLLATE_MASK | LC_CTYPE_MASK | LC_MONETARY_MASK | LC_NUMERIC_MASK | LC_TIME_MASK | LC_MESSAGES_MASK)

typedef struct locale_struct *locale_t;

#define LC_GLOBAL_LOCALE ((locale_t) -1)

__Z_EXPORT extern locale_t __c_locale() __asm("__zlc_locale_a");
__Z_EXPORT extern locale_t newlocale(int category_mask, const char* locale, locale_t base) __asm("__zlnewlocale_a");
__Z_EXPORT extern void freelocale(locale_t locobj) __asm("__zlfreelocale");
__Z_EXPORT extern locale_t uselocale(locale_t newloc) __asm("__zluselocale_a");
__Z_EXPORT extern const char *getlocalename_l(int category, locale_t locale);

#if defined(__cplusplus)
}
#endif

#else
#include_next <locale.h>

#endif 

#endif
