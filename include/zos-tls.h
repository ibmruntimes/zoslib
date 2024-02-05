///////////////////////////////////////////////////////////////////////////////
// Licensed Materials - Property of IBM
// ZOSLIB
// (C) Copyright IBM Corp. 2020. All Rights Reserved.
// US Government Users Restricted Rights - Use, duplication
// or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
///////////////////////////////////////////////////////////////////////////////

// TODO(gabylb): zos - revisit when thread_local is implemented.
// APIs that implement Thread-local storage support.

#ifndef ZOS_TLS_H_
#define ZOS_TLS_H_

#include "zos-macros.h"

#ifdef __cplusplus
#include <unistd.h>

/**TODO(itodorov) - zos: document these interfaces **/
struct __Z_EXPORT __tlsanchor;
__Z_EXPORT struct __tlsanchor* __tlsvaranchor_create(size_t sz);
__Z_EXPORT void __tlsvaranchor_destroy(struct __tlsanchor *anchor);
__Z_EXPORT void* __tlsPtrFromAnchor(struct __tlsanchor *anchor, const void *);

template <typename T> class __Z_EXPORT __tlssim {
  struct __tlsanchor *anchor;
  T v;

public:
  __tlssim(const T &initvalue) : v(initvalue) {
    anchor = __tlsvaranchor_create(sizeof(T));
  }
  __tlssim() { anchor = __tlsvaranchor_create(sizeof(T)); }
  ~__tlssim() { __tlsvaranchor_destroy(anchor); }
  T *access(void) { return static_cast<T *>(__tlsPtrFromAnchor(anchor, &v)); }
};

#endif // __cplusplus

/* 
These macros provide a workaround for the non-existing thread local storage (TLS) 
support for C applications on z/os. It leverages the pthread_key_create, pthread_getspecific 
and pthread_setspecific calls to acheve TLS.
*/

static void tls_destructor(void *value) {
    free(value);
}

#define GET_KEY_VAL(key_name, key_status, val, size) \
   if(key_status == 0){\
      int resp = pthread_key_create(&key_name, tls_destructor);  \
      assert(resp == 0);\
      key_status = 1;\
   } \
   val = pthread_getspecific(key_name); \
    if (val == NULL) \
    { \
        val = calloc(1,size);\
        assert(val != NULL);\
    }

#define SET_KEY_VAL(key_name,val) \
pthread_setspecific(key_name,val);

#define GET_KEY_VAL_ARRAY(key_name,key_status, val, size, rows, columns)\
GET_KEY_VAL(key_name,key_status, val, ((rows*sizeof(char *))+(rows*columns*size))) \
{\
char *data_start = (char *)(val + rows);\
    for (int i = 0; i < rows; i++) { \
        val[i] = data_start + i * columns;\
    } \
}
#endif // ZOS_TLS_H_
