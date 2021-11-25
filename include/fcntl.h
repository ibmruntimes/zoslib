#ifndef ZOS_FCNTL_H_
#define ZOS_FCNTL_H_

#if defined(ZOSLIB_OVERRIDE_CLIB) || defined(ZOSLIB_OVERRIDE_CLIB_FNCTL)

#undef open
#define open __open_replaced
#include_next <fcntl.h>
#undef open

#if defined(__cplusplus)
extern "C" {
#endif
int open(const char *filename, int opts, ...) asm("__open_ascii");
#if defined(__cplusplus)
};
#endif

#else

#include_next <fcntl.h>

#endif

#endif
