#ifndef ZOS_UNISTD_H_
#define ZOS_UNISTD_H_

#if defined(ZOSLIB_OVERRIDE_CLIB) || defined(ZOSLIB_OVERRIDE_CLIB_UNISTD)

#undef pipe 
#define pipe __pipe_replaced
#include_next <unistd.h>
#undef pipe

#if defined(__cplusplus)
extern "C" {
#endif
int pipe(int [2]) asm("__pipe_ascii");
#if defined(__cplusplus)
};
#endif

#else

#include_next <unistd.h>

#endif

#endif
