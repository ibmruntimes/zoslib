#ifndef __ZOS_GET_ENTROPY_H_
#define __ZOS_GET_ENTROPY_H_

#if (__EDC_TARGET < 0x42050000) || defined(ZOSLIB_ENABLE_V2R5_FEATURES)
#include "zos-macros.h"
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif
/**
 * Fill a buffer with random bytes
 * \param [out] buffer to store random bytes to.
 * \param [in] number of random bytes to generate.
 * \return On success, returns 0, or -1 on error.
 */
#if (__EDC_TARGET < 0x42050000) && defined(ZOSLIB_ENABLE_V2R5_FEATURES)
__Z_EXPORT extern int (*getentropy)(void *, size_t);
__Z_EXPORT int __getentropy(void* buffer, size_t length);
#else
__Z_EXPORT int getentropy(void* buffer, size_t length);
#endif

#ifdef __cplusplus
}
#endif

#endif // __ZOS_GET_ENTROPY_H_
#endif // (__EDC_TARGET < 0x42050000) || defined(ZOSLIB_ENABLE_V2R5_FEATURES)
