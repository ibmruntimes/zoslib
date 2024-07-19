///////////////////////////////////////////////////////////////////////////////
// Licensed Materials - Property of IBM
// ZOSLIB
// (C) Copyright IBM Corp. 2024. All Rights Reserved.
// US Government Users Restricted Rights - Use, duplication
// or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
///////////////////////////////////////////////////////////////////////////////

#ifdef ZOSLIB_TRACE_ALLOCS
#include "zos-tsearch.h"
#include "zos-base.h"

#include <assert.h>
#include <errno.h>
#include <libgen.h>
#include <search.h>
#include <stdlib.h>
#include <string.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))

// These are used to track the allocated memory for the btrees.
static size_t gBytesCurrent= 0u;
static size_t gBytesMaxAllocated = 0u;
// Used when writing the stats report:
static int gMaxSrcNameLen = 0;

size_t __get_btree_bytes_current() { return gBytesCurrent; }
size_t __get_btree_bytes_max() { return gBytesMaxAllocated; }
int __get_btree_max_src_namelen() { return gMaxSrcNameLen; }


int get_num_len(int linenum) {
  int x = 0;
  if (linenum < 0) {
    linenum = abs(linenum);
    x = 1;
  }
  int len = 1;
  while(linenum > 9) {
    ++len;
    linenum /= 10;
  }
  return len + x;
}

const char *__bt_memspace_str(__MEMSPACE memspace) {
  switch(memspace) {
    case __MEMSPACE_64: {
      static const char s[] = "64";
      return s;
    } case __MEMSPACE_64V: {
      static const char s[] = "VS";
      return s;
    } case __MEMSPACE_31: {
      static const char s[] = "31";
      return s;
    } default: {
      static const char s[] = "UNKNOWN-ADDRESS-SPACE";
      return s;
    }
  }
}

//------------------------------------------------------------------------------
// __bt_addr_* store each memory address allocated as a node in a btree.
//
void __bt_addr_free_node(__taddr_t *pn) {
  if (!pn)
    return;
  if (pn->psrc) {
    gBytesCurrent -= (strlen(pn->psrc) + 1);
    __free_orig(pn->psrc);
  }
  gBytesCurrent -= sizeof(*pn);
  __free_orig(pn);
}

int __bt_addr_compare(const void *node1, const void *node2) {
  const void *addr1 = ((const __taddr_t*)node1)->addr;
  const void *addr2 = ((const __taddr_t*)node2)->addr;
  if (addr1 < addr2) return -1;
  if (addr1 == addr2) return 0;
  if (addr1 > addr2) return 1;
  assert(0);
  return 0;
}

void __bt_addr_add(void **pproot_addr, void **pproot_src, __taddr_t **ppnode,
                   __MEMSPACE memspace, const void *addr, size_t nbytes,
                   const char *pfname, int linenum) {
  int linenum_len = get_num_len(linenum);
  if (linenum_len <= 0) {
    linenum = 0;
    linenum_len = 1;
  }
  __taddr_t *pn = (__taddr_t*)__malloc_orig(sizeof(__taddr_t));
  gBytesCurrent += sizeof(__taddr_t);
  pn->addr = addr;
  pn->nbytes = nbytes;
  pn->memspace = memspace;
  const char *bn = __file_basename(pfname);
  int len = strlen(bn) + linenum_len + 1;
  gMaxSrcNameLen = MAX(gMaxSrcNameLen, len);
  pn->psrc = (char*)__malloc_orig(len + 1);
  pn->callnum = 1;
  gBytesCurrent += len + 1;
  gBytesMaxAllocated = MAX(gBytesMaxAllocated, gBytesCurrent);
  snprintf(pn->psrc, len + 1, "%s:%d", bn, linenum);

  if (ppnode != NULL)
    *ppnode = NULL;
  void *node = tsearch((void*)pn, pproot_addr, __bt_addr_compare);
  if (node == NULL) {
    __memprintf("ERROR in __bt_addr_add:%d tsearch failed: errno=%d\n",
                __LINE__, errno);
    abort();
  } else if (*(__taddr_t**)node != pn) {
    __memprintf("ERROR in __bt_addr_add:%d addr=%p already exists\n",
                __LINE__, addr);
    abort();
  } else {
    __bt_src_add(pproot_src, pn);
    if (ppnode != NULL)
      *ppnode = pn;
  }
}

void __bt_addr_delete(void **pproot_addr, void **pproot_src, const void *addr) {
  if (pproot_addr == NULL || *pproot_addr == NULL) {
    __memprintf("ERROR in __bt_addr_delete:%d addr=%p: pproot_addr=%p is NULL\n",
                __LINE__, addr, pproot_addr);
    abort();
  }
  __taddr_t *paddr_node = __bt_addr_find(pproot_addr, addr);
  if (paddr_node == NULL) {
    __memprintf("ERROR in __bt_addr_delete:%d pproot_addr=%p addr=%p not found\n",
                __LINE__, pproot_addr, addr);
    abort();
  }
  __bt_src_dec(pproot_src, paddr_node);
  void *pret = tdelete(paddr_node, pproot_addr, __bt_addr_compare);
  if (pret == NULL) {
    __memprintf("ERROR in __bt_addr_delete:%d: addr=%p not found.\n", __LINE__, addr);
    abort();
  }
  __bt_addr_free_node(paddr_node);
}

__taddr_t *__bt_addr_find(void **pproot, const void *addr) {
  __taddr_t node;
  node.addr = addr;
  void *pnode = tfind((const void*)&node, pproot, __bt_addr_compare);
  if (pnode == NULL)
    return NULL;
  return *(__taddr_t**)pnode;
}

//------------------------------------------------------------------------------
// __bt_src_* store the source file locations (base filename:line-num) where each
// memory allocation call was made, for the stats report.

void __bt_src_free_node(__tsrc_t *pn) {
  if (!pn)
    return;
  if (pn->psrc) {
    gBytesCurrent -= (strlen(pn->psrc) + 1);
    __free_orig(pn->psrc);
  }
  gBytesCurrent -= sizeof(*pn);
  __free_orig(pn);
}

int __bt_src_compare(const void *node1, const void *node2) {
  const char *psrc1 = ((const __tsrc_t*)node1)->psrc;
  const char *psrc2 = ((const __tsrc_t*)node2)->psrc;
  return strcmp(psrc1, psrc2);
}

void __bt_src_add(void **pproot, __taddr_t *paddr_node) {
  __tsrc_t *pn = (__tsrc_t*)__malloc_orig(sizeof(__tsrc_t));
  gBytesCurrent += sizeof(__tsrc_t);
  pn->psrc = __strdup_orig(paddr_node->psrc);
  gBytesCurrent += strlen(pn->psrc) + 1;
  gBytesMaxAllocated = MAX(gBytesMaxAllocated, gBytesCurrent);
  pn->curbytes = paddr_node->nbytes;
  pn->maxbytes = paddr_node->nbytes;
  pn->memspace = paddr_node->memspace;
  pn->lastrpt_curbytes = 0u;
  pn->nallocs = 1u;
  pn->nfrees = 0u;

  void *node = tsearch((void*)pn, pproot, __bt_src_compare);
  if (node == NULL) {
    __memprintf("ERROR in __bt_src_add:%d tsearch failed: errno=%d\n",
                __LINE__, errno);
    abort();
  } else if (*(__tsrc_t**)node != pn) {
    // a node containing this source location already exists, update it.
    __bt_src_free_node(pn);
    pn = *(__tsrc_t**)node;
    assert(pn->memspace == paddr_node->memspace);
    pn->curbytes += paddr_node->nbytes;
    pn->maxbytes = MAX(pn->maxbytes, pn->curbytes);
    pn->nallocs++;
    paddr_node->callnum = pn->nallocs;
  }
}

void __bt_src_dec(void **pproot_src, const __taddr_t *paddr_node) {
  // Called from __bt_addr_delete after an address is freed, to update
  // the alloc info of the node in the src tree from which the allocation
  // call was made.
  __tsrc_t *pnode = __bt_src_find(pproot_src, paddr_node->psrc);
  if (pnode == NULL) {
    __memprintf("ERROR in __bt_src_dec:%d: src=%s not found.\n",
                __LINE__, paddr_node->psrc);
    return;
  }
  pnode->curbytes -= paddr_node->nbytes;
  pnode->nfrees++;
}

__tsrc_t *__bt_src_find(void **pproot, char *psrc) {
  __tsrc_t node;
  node.psrc = psrc;
  void *pnode = tfind((const void*)&node, pproot, __bt_src_compare);
  if (pnode == NULL)
    return NULL;
  return *(__tsrc_t**)pnode;
}
#endif // ZOSLIB_TRACE_ALLOCS
