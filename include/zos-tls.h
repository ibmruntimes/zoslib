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

#ifdef __cplusplus
#include <unistd.h>

/**TODO(itodorov) - zos: document these interfaces **/
struct __tlsanchor;
struct __tlsanchor *__tlsvaranchor_create(size_t sz);
void __tlsvaranchor_destroy(struct __tlsanchor *anchor);
void *__tlsPtrFromAnchor(struct __tlsanchor *anchor, const void *);

template <typename T> class __tlssim {
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
#endif // ZOS_TLS_H_
