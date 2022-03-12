#ifndef __ZOS_GET_ENTROPY_H_
#define __ZOS_GET_ENTROPY_H_

#ifdef __cplusplus
extern "C" {
#endif
extern int getentropy(void* buffer, size_t length);
#ifdef __cplusplus
}
#endif

#endif // __ZOS_GET_ENTROPY_H_
