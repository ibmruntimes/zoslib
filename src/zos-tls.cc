///////////////////////////////////////////////////////////////////////////////
// Licensed Materials - Property of IBM
// ZOSLIB
// (C) Copyright IBM Corp. 2020. All Rights Reserved.
// US Government Users Restricted Rights - Use, duplication
// or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
///////////////////////////////////////////////////////////////////////////////

#define _POSIX_SOURCE
#include "zos-tls.h"

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static void _cleanup(void *p) {
  pthread_key_t key = *((pthread_key_t *)p);
  free(p);
  pthread_setspecific(key, 0);
}

static void *__tlsPtrAlloc(size_t sz, pthread_key_t *k, pthread_once_t *o,
                           const void *initvalue) {
  unsigned int initv = 0;
  unsigned int expv;
  unsigned int newv = 1;
  expv = 0;
  newv = 1;
  __asm volatile(" cs  %0,%2,%1 \n"
                 : "+r"(expv), "+m"(*o)
                 : "r"(newv)
                 :);
  initv = expv;
  if (initv == 2) {
    // proceed
  } else if (initv == 0) {
    // create
    pthread_key_create(k, _cleanup);
    expv = 1;
    newv = 2;
    __asm volatile(" cs  %0,%2,%1 \n"
                   : "+r"(expv), "+m"(*o)
                   : "r"(newv)
                   :);
    initv = expv;
  } else {
    // wait and poll for completion
    while (initv != 2) {
      expv = 0;
      newv = 1;
      __asm volatile(" la 15,0\n"
                     " svc 137\n"
                     " cs  %0,%2,%1 \n"
                     : "+r"(expv), "+m"(*o)
                     : "r"(newv)
                     : "r15", "r6");
      initv = expv;
    }
  }
  void *p = pthread_getspecific(*k);
  if (!p) {
    // first call in thread allocate
    p = malloc(sz + sizeof(pthread_key_t));
    memcpy(p, k, sizeof(pthread_key_t));
    pthread_setspecific(*k, p);
    memcpy((char *)p + sizeof(pthread_key_t), initvalue, sz);
  }
  return (char *)p + sizeof(pthread_key_t);
}

#if 0
// Currently unused.
static void *__tlsPtr(pthread_key_t *key) { return pthread_getspecific(*key); }
static void __tlsDelete(pthread_key_t *key) { pthread_key_delete(*key); }
#endif


struct __tlsanchor *__tlsvaranchor_create(size_t sz) {
  struct __tlsanchor *a =
      (struct __tlsanchor *)calloc(1, sizeof(struct __tlsanchor));
  a->once = PTHREAD_ONCE_INIT;
  a->sz = sz;
  return a;
}

void __tlsvaranchor_destroy(struct __tlsanchor *anchor) {
  pthread_key_delete(anchor->key);
  free(anchor);
}

void *__tlsPtrFromAnchor(struct __tlsanchor *anchor, const void *initvalue) {
  return __tlsPtrAlloc(anchor->sz, &(anchor->key), &(anchor->once), initvalue);
}
void * __tlsValue(tls_t *a) {
  void *val = NULL;
  char * initvalue = (char *)malloc(sizeof(char)*(a->sz));
  assert(initvalue != NULL);
  bzero(initvalue,a->sz);
  val = __tlsPtrAlloc(a->sz, &(a->key), &(a->once),(void *)initvalue);
  free(initvalue);
  return val;
}

char** __tlsArray(tls_t *a, int rows, int columns) {
  char **val = NULL;
  a->sz = ((rows*sizeof(char *))+(rows*columns*(a->sz)));
  val = (char **)__tlsValue(a);
  for (int i = 0; i < rows; i++) {
    val[i] = (char*)(val + rows) + i * columns;
  }
  return val;
}

