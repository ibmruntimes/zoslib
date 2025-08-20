///////////////////////////////////////////////////////////////////////////////
// Licensed Materials - Property of IBM
// ZOSLIB
// (C) Copyright IBM Corp. 2024. All Rights Reserved.
// US Government Users Restricted Rights - Use, duplication
// or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
///////////////////////////////////////////////////////////////////////////////

#ifndef ZOS_TSEARCH_H_
#define ZOS_TSEARCH_H_

#if defined(ZOSLIB_TRACE_ALLOCS)

#include <sys/types.h>

typedef enum {
  __MEMSPACE_64 = 1,
  __MEMSPACE_64V,
  __MEMSPACE_31
} __MEMSPACE;

typedef struct {
  const void *addr;
  size_t nbytes;
  char *psrc;
  size_t callnum;  // the i'th time an alloc from location psrc was made
  __MEMSPACE memspace;
} __taddr_t;

typedef struct {
  char *psrc;
  size_t curbytes; // # of bytes currently still allocated from this location
  size_t maxbytes; // max # of bytes allocated from this location
  size_t nallocs;  // # of times an alloc has been called from this location
  size_t nfrees;   // # of times an alloc from this location has been freed
  size_t lastrpt_curbytes; // last curbytes value displayed for stats
  __MEMSPACE memspace;
} __tsrc_t;

#if defined(__cplusplus)
extern "C" {
#endif

const char *__bt_memspace_str(__MEMSPACE memspace);

// __bt_addr_* manipulate the binary search tree with memory address as key
void __bt_addr_add(void **pproot_addr, void **pproot_src, __taddr_t **ppnode,
                   __MEMSPACE memspace,
                   const void *addr, size_t nbytes,
                   const char *pfname, int linenum);
void __bt_addr_delete(void **pproot_addr, void **pproot_src,
                      const void *addr);
int  __bt_addr_compare(const void *node1, const void *node2);
void __bt_addr_free_node( __taddr_t *pn);
__taddr_t *__bt_addr_find(void **pproot, const void *addr);

// __bt_src_* manipulate the binary search tree with alloc source:linenum as key
void __bt_src_add(void **pproot, __taddr_t *paddr_node);
int  __bt_src_compare(const void *node1, const void *node2);
void __bt_src_free_node( __tsrc_t *pn);
void __bt_src_dec(void **pproot_src, const __taddr_t *paddr_node);
__tsrc_t *__bt_src_find(void **pproot, char *psrc);


#if defined(__cplusplus)
}
#endif

#endif
#endif
