///////////////////////////////////////////////////////////////////////////////
// Licensed Materials - Property of IBM
// ZOSLIB
// (C) Copyright IBM Corp. 2024. All Rights Reserved.
// US Government Users Restricted Rights - Use, duplication
// or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
///////////////////////////////////////////////////////////////////////////////

#define _AE_BIMODAL 1
#include "zos-base.h"
#include "zos-io.h"
#include "zos-tsearch.h"

#include <algorithm>

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <search.h>
#include <stdlib.h>
#include <string.h>

extern "C" bool __isZoslibInitialized();

extern "C" void heapreport() {
  // Called first from __zinit::__zinit (after envars for memory trace are
  // processed), and again after the report has been generated by
  // __display_alloc_stats().
  static long start__uheap_bytes_alloc = -1;
  hreport_t r;
  if (__heaprpt(&r) != 0) {
    int sverrno = errno;
    perror("__heaprpt");
    __memprintfx("ERROR: __heaprpt() failed errno=%d\n", sverrno);
    return;
  }
  char ts[20] = "(error)"; // yyyy-mm-dd hh:mm:ss
  __get_timestamp(ts);
  if (start__uheap_bytes_alloc < 0) {
    __memprintfx("__heaprpt() at %s:\n" \
                 "  Total amount of user heap storage    : %ld\n" \
                 "  Amount of user heap storage in use   : %ld\n" \
                 "  Amount of available user heap storage: %ld\n\n", ts,
                 r.__uheap_size, r.__uheap_bytes_alloc, r.__uheap_bytes_free);
  } else {
    char s[64] = "";
    if (r.__uheap_bytes_alloc > start__uheap_bytes_alloc)
      snprintf(s, sizeof(s), "(+%zu vs start %zu)", r.__uheap_bytes_alloc -
               start__uheap_bytes_alloc, start__uheap_bytes_alloc);
    else if (r.__uheap_bytes_alloc < start__uheap_bytes_alloc)
      snprintf(s, sizeof(s), "(%ld vs start %zu)", r.__uheap_bytes_alloc -
               start__uheap_bytes_alloc, start__uheap_bytes_alloc);

    __memprintfx("__heaprpt() at %s:\n" \
                 "  Total amount of user heap storage    : %ld\n" \
                 "  Amount of user heap storage in use   : %ld%s\n" \
                 "  Amount of available user heap storage: %ld\n\n", ts,
                 r.__uheap_size, r.__uheap_bytes_alloc, s,
                 r.__uheap_bytes_free);
  }
  if (start__uheap_bytes_alloc < 0)
    start__uheap_bytes_alloc = r.__uheap_bytes_alloc;
}


// For use by IARV64 allocation/free:
namespace {
char gxttoken[16] = "";
unsigned short asid;
bool oktouse;

void getxttoken(char *out) {
  void *p;
  asm volatile("  l %0,1208 \n"
               "  llgtr %0,%0 \n"
               "  lg %0,304(%0)\n"
               : "+r"(p)
               :
               : "r0");
  memcpy(out, (char *)p + 0x14, 16);
}

__attribute__((constructor)) void init_iarv64() {
  getxttoken(gxttoken);
  asid = ((unsigned short *)(*(char *__ptr32 *)(0x224)))[18];

  // LE level is 220 or above
  oktouse =
     (*(int *)(80 + ((char ****__ptr32 *)1208)[0][11][1][123]) >= 0x04020200);
  assert(oktouse);
}
} // namespace

#if defined(ZOSLIB_TRACE_ALLOCS)
namespace {
/*
 * It's possible that the overridden allocation functions here get called
 * before the constructors init_* are invoked (e.g. during init of static
 * objects), hence the check for access_lock before it's used.
*/
pthread_mutex_t access_lock = {0};

// Display tracaback for warnings if envar is set:
bool gDoWarningTB = false;

// Determines if only changed src nodes should be displayed (display_src); it's
// global because it's used by display_src callback funciton.
bool gbDisplayAllAllocStats = true;

bool doTraceback(const char *pfname_linenum) {
  // for debugging
  static bool binit = false;
  static char srclinebuf[PATH_MAX] = "";
  static unsigned long fromi = 0u, toj = 0u;
  static unsigned long curi = 0u;
  if (!binit) {
    const char *penv = getenv("__MEMORY_USAGE_ALLOC_TB_SOURCE");
    if (penv && *penv)
      strncpy(srclinebuf, penv, sizeof(srclinebuf));

    penv = getenv("__MEMORY_USAGE_ALLOC_TB_SOURCE_I");
    if (penv && *penv) {
      sscanf(penv, "%ld", &fromi);
      penv = getenv("__MEMORY_USAGE_ALLOC_TB_SOURCE_J");
      if (penv && *penv)
        sscanf(penv, "%ld", &toj);
      if (fromi > toj)
        toj = fromi;
    }
    binit = true;
  }
  if (!*srclinebuf)
    return false;
  if (strcmp(srclinebuf, pfname_linenum) != 0)
    return false;
  if (fromi == 0u && toj == 0u)
    return true;
  else if (fromi > 0u && toj >= fromi) {
    if (++curi >= fromi && curi <= toj)
      return true;
  } else if (fromi > 0u && ++curi >= fromi)
    return true;
 
  return false;
}

extern "C" size_t __get_btree_bytes_current();
extern "C" size_t __get_btree_bytes_max();
extern "C" int __get_btree_max_src_namelen();

// These are updated by display_node() comparator when display_debris() runs.
size_t gUnfreed64, gCountAllocs64, gCountFrees64;
size_t gUnfreed64v, gCountAllocs64v, gCountFrees64v;
size_t gUnfreed31, gCountAllocs31, gCountFrees31;


void display_node(const void *ptr, VISIT order, int level) {
  const __taddr_t *p = *(const __taddr_t**) ptr;

  if (order == postorder || order == leaf)  {
    __memprintfx("DEBRIS from %s-%d addr=%lX size=%lu\n",
                p->psrc, p->callnum, p->addr, p->nbytes);
  }
}

void display_src(const void *ptr, VISIT order, int level) {
  __tsrc_t *p = *(__tsrc_t**) ptr;
  static int max_width = __get_btree_max_src_namelen();

  if (order != postorder && order != leaf)
    return;

  if (p->memspace == __MEMSPACE_64) {
    gUnfreed64 += p->curbytes;
    gCountAllocs64 += p->nallocs;
    gCountFrees64 += p->nfrees;
  } else if (p->memspace == __MEMSPACE_64V) {
    gUnfreed64v += p->curbytes;
    gCountAllocs64v += p->nallocs;
    gCountFrees64v += p->nfrees;
  } else if (p->memspace == __MEMSPACE_31) {
    gUnfreed31 += p->curbytes;
    gCountAllocs31 += p->nallocs;
    gCountFrees31 += p->nfrees;
  } else
    assert(0);

  if (gbDisplayAllAllocStats) {
    __memprintfx("%s %*s: unfreed=%zu max-allocated=%zu "\
                 "count-allocations=%zu count-frees=%zu count-diff=%zu\n",
                 __bt_memspace_str(p->memspace), max_width, p->psrc,
                 p->curbytes, p->maxbytes, p->nallocs,
                 p->nfrees, p->nallocs - p->nfrees);
    p->lastrpt_curbytes = p->curbytes;
  } else if (p->lastrpt_curbytes != p->curbytes) {
    char s[64] = "";
    if (p->curbytes > p->lastrpt_curbytes)
      snprintf(s, sizeof(s), "(+%zu)", p->curbytes - p->lastrpt_curbytes);
    else if (p->curbytes < p->lastrpt_curbytes)
      snprintf(s, sizeof(s), "(%ld)", (signed long)p->curbytes - \
               (signed long)p->lastrpt_curbytes);

    __memprintfx("%s %*s: unfreed=%zu%s max-allocated=%zu " \
                 "count-allocations=%zu count-frees=%zu " \
                 "count-diff=%zu\n",
                 __bt_memspace_str(p->memspace), max_width, p->psrc,
                 p->curbytes, s, p->maxbytes, p->nallocs, p->nfrees,
                 p->nallocs - p->nfrees);

    p->lastrpt_curbytes = p->curbytes;
  }
}

int delete_root(const void *node1, const void *node2) {
  // Used when explicitly deleting a root node, so no comparison is needed.
  return 0;
}

size_t curheap64 = 0u;     // 64-bit heap malloc, calloc, etc.
size_t maxheap64 = 0u;     // also includes allocations from C++ new
size_t lastvaldisp64 = 0u; // last curheap64 value that was displayed
size_t lastrptheap64 = 0u; // last curheap64 value displayed for stats

size_t curmem31 = 0u;      // memory below the bar
size_t maxmem31 = 0u;
size_t lastvaldisp31 = 0u;
size_t lastrptmem31 = 0u;

size_t curmem64v = 0u;     // 64-bit virtual storage (IARV64)
size_t maxmem64v = 0u;
size_t lastvaldisp64v = 0u;
size_t lastrptmem64v = 0u;

void *proot_addr = NULL; // btree root of allocated addresses
void *proot_src = NULL;  // btree root of allocations source (filename:linenum)


void addptr(__taddr_t **ppn, const __MEMSPACE memspace, const void *ptr,
            size_t v, const char *pfname, int linenum, bool *pisCached = NULL) {
  if (access_lock.__m > 0 && pthread_mutex_lock(&access_lock) != 0) {
    perror("pthread_mutex_lock");
    abort();
  }
  if (pisCached)
    *pisCached = false;
  __taddr_t *pn = __bt_addr_find(&proot_addr, ptr);
  if (pn != NULL) {
    assert(pn->addr == ptr);
    if (pisCached)
      *pisCached = true;
    else if (__doLogMemoryWarning()) {
      __memprintf("WARN addr=%p size=%zu memspace=%d already in cache, " \
                  "new-size=%zu memspace=%d (%s)\n",
                  ptr, pn->nbytes, pn->memspace, v, memspace, pn->psrc);
      if (gDoWarningTB)
        __display_backtrace(__getLogMemoryFileNo());
    }
  } else {
    __bt_addr_add(&proot_addr, &proot_src, &pn, memspace, ptr, v, pfname,
                  linenum);
    if (memspace == __MEMSPACE_64) {
      curheap64 += v;
      maxheap64 = std::max(maxheap64, curheap64);
    } else if (memspace == __MEMSPACE_64V) {
      curmem64v += v;
      maxmem64v = std::max(maxmem64v, curmem64v);
    } else if (memspace == __MEMSPACE_31) {
      curmem31 += v;
      maxmem31 = std::max(maxmem31, curmem31);
    } else {
      assert(0);
    }
  }
  assert (pn != NULL);
  if (access_lock.__m > 0 && pthread_mutex_unlock(&access_lock) != 0) {
    perror("pthread_mutex_unlock");
    abort();
  }
  *ppn = pn;
}

size_t freeptr(const void *ptr) {
  if (access_lock.__m > 0 && pthread_mutex_lock(&access_lock) != 0) {
    perror("pthread_mutex_lock");
    abort();
  }
  __taddr_t *pn = __bt_addr_find(&proot_addr, ptr);
  size_t size = 0u;
  if (pn != NULL) {
    size = pn->nbytes;
    if (pn->memspace == __MEMSPACE_64) {
      curheap64 -= size;
    } else if (pn->memspace == __MEMSPACE_64V) {
      curmem64v -= size;
    } else if (pn->memspace == __MEMSPACE_31) {
      curmem31 -= size;
    } else {
      assert(0);
    }
    __bt_addr_delete(&proot_addr, &proot_src, ptr);
  } else if (__doLogMemoryWarning()) {
    __memprintf("WARN free addr=%p not in cache, return size 0\n", ptr);
    if (gDoWarningTB)
      __display_backtrace(__getLogMemoryFileNo());
  }
  if (access_lock.__m > 0 && pthread_mutex_unlock(&access_lock) != 0) {
    perror("pthread_mutex_unlock");
    abort();
  }
  return size;
}

size_t getsize(const void *ptr, const char *pfname, int linenum) {
  if (access_lock.__m > 0 && pthread_mutex_lock(&access_lock) != 0) {
    perror("pthread_mutex_lock");
    abort();
  }
  __taddr_t *pn = __bt_addr_find(&proot_addr, ptr);
  size_t size = 0u;
  if (pn != NULL)
    size = pn->nbytes;
  if (access_lock.__m > 0 && pthread_mutex_unlock(&access_lock) != 0) {
    perror("pthread_mutex_unlock");
    abort();
  }
  if (pn != NULL)
    return size;
  if (__doLogMemoryWarning()) {
    __memprintf("WARN getsize addr=%p not in cache, returning size 0 (%s:%d)\n",
                ptr, __file_basename(pfname), linenum);
    if (gDoWarningTB)
      __display_backtrace(__getLogMemoryFileNo());
  }
  return 0u;
}

void display_stats() {
  if (!__doLogMemoryUsage())
    return;

  if (access_lock.__m > 0 && pthread_mutex_lock(&access_lock) != 0) {
    perror("pthread_mutex_lock");
    abort();
  }

  char ts[20] = "(error)"; // yyyy-mm-dd hh:mm:ss
  __get_timestamp(ts);
  if (!gbDisplayAllAllocStats) {
    __memprintfx("\nMEMORY ALLOCATIONS (only those with an updated 'unfreed')" \
                 " at %s:\n", ts);
  } else {
    __memprintfx("\nMEMORY ALLOCATIONS at %s:\n", ts);
  }

  if (proot_addr && __doLogMemoryWarning()) {
    // display all unfreed pointers
    twalk(proot_addr, display_node);
  }

  if (!proot_src)
    return;

  // These global variables are updated by display_src:
  gUnfreed64 = gCountAllocs64 = gCountFrees64 = 0u;
  gUnfreed64v = gCountAllocs64v = gCountFrees64v = 0u;
  gUnfreed31 = gCountAllocs31 = gCountFrees31 = 0u;

  twalk(proot_src, display_src);

  char w[64] = "";
  char s64[64] = "";
  char s64v[64] = "";
  char s31[64] = "";

  if (gUnfreed64 != curheap64)
    sprintf(w, " (WARN curheap64=%zu)", curheap64);
  if (gbDisplayAllAllocStats) {
    __memprintfx("\nTOTAL     64-heap: unfreed=%zu%s " \
                 "max-allocated=%zu count-allocations=%zu count-frees=%zu " \
                 "count-diff=%zu\n", gUnfreed64, w, maxheap64,
                 gCountAllocs64, gCountFrees64,
                 gCountAllocs64 - gCountFrees64);
  } else {
    if (curheap64 > lastrptheap64)
      snprintf(s64, sizeof(s64), "(+%zu)", curheap64 - lastrptheap64);
    else if (curheap64 < lastrptheap64)
      snprintf(s64, sizeof(s64), "(%ld)", (signed long)curheap64 - \
             (signed long)lastrptheap64);

    __memprintfx("\nTOTAL     64-heap: unfreed=%zu%s%s " \
                 "max-allocated=%zu count-allocations=%zu count-frees=%zu " \
                 "count-diff=%zu\n", gUnfreed64, s64, w, maxheap64,
                 gCountAllocs64, gCountFrees64,
                 gCountAllocs64 - gCountFrees64);
  }

  *w = 0;
  if (gUnfreed64v != curmem64v)
    sprintf(w, " (WARN curheap64v=%zu)", curmem64v);
  if (gbDisplayAllAllocStats) {
    __memprintfx("TOTAL       64-vs: unfreed=%zu%s " \
                 "max-allocated=%zu count-allocations=%zu count-frees=%zu " \
                 "count-diff=%zu\n", gUnfreed64v, w, maxmem64v,
                 gCountAllocs64v, gCountFrees64v,
                 gCountAllocs64v - gCountFrees64v);
  } else {
    if (curmem64v > lastrptmem64v)
      snprintf(s64v, sizeof(s64v), "(+%zu)", curmem64v - lastrptmem64v);
    else if (curmem64v < lastrptmem64v)
      snprintf(s64v, sizeof(s64v), "(%ld)", (signed long)curmem64v - \
             (signed long)lastrptmem64v);

    __memprintfx("TOTAL       64-vs: unfreed=%zu%s%s " \
                 "max-allocated=%zu count-allocations=%zu count-frees=%zu " \
                 "count-diff=%zu\n", gUnfreed64v, s64v, w, maxmem64v,
                 gCountAllocs64v, gCountFrees64v,
                 gCountAllocs64v - gCountFrees64v);
  }

  *w = 0;
  if (gUnfreed31 != curmem31)
    sprintf(w, " (WARN curmem31=%zu)", curmem31);
  if (gbDisplayAllAllocStats) {
    __memprintfx("TOTAL      31-bit: unfreed=%zu%s " \
                 "max-allocated=%zu count-allocations=%zu count-frees=%zu " \
                 "count-diff=%zu\n", gUnfreed31, w, maxmem31,
                 gCountAllocs31, gCountFrees31,
                 gCountAllocs31 - gCountFrees31);
  } else {
    if (curmem31 > lastrptmem31)
      snprintf(s31, sizeof(s31), "(+%zu)", curmem31 - lastrptmem31);
    else if (curmem31 < lastrptmem31)
      snprintf(s31, sizeof(s31), "(%ld)", (signed long)curmem31 - \
             (signed long)lastrptmem31);

    __memprintfx("TOTAL      31-bit: unfreed=%zu%s%s " \
                 "max-allocated=%zu count-allocations=%zu count-frees=%zu " \
                 "count-diff=%zu\n", gUnfreed31, s31, w, maxmem31,
                 gCountAllocs31, gCountFrees31,
                 gCountAllocs31 - gCountFrees31);
  }

  __memprintfx("\n");
  // summary:
  size_t h64 = curheap64;
  if (gbDisplayAllAllocStats) {
    __memprintfx("SUMMARY: unfreed64=%zu, max64=%zu, " \
                 "unfreed64v=%zu, max64v=%zu, unfreed31=%zu, max31=%zu\n",
                 curheap64, maxheap64, curmem64v, maxmem64v, curmem31, maxmem31);
  } else {
    __memprintfx("SUMMARY: unfreed64=%zu%s, max64=%zu, " \
                 "unfreed64v=%zu%s, max64v=%zu, unfreed31=%zu%s, max31=%zu\n",
                 curheap64, s64, maxheap64, curmem64v, s64v, maxmem64v,
                 curmem31, s31, maxmem31);
  }
  lastrptheap64 = curheap64;
  lastrptmem64v = curmem64v;
  lastrptmem31 = curmem31;

  if (access_lock.__m > 0 && pthread_mutex_unlock(&access_lock) != 0) {
    perror("pthread_mutex_unlock");
    abort();
  }
}

void destroy_nodes() {
  // delete all addr tree nodes:
  __taddr_t *paddr_node;
  while (proot_addr != NULL) {
    paddr_node = *(__taddr_t**)proot_addr;
    tdelete((void *)paddr_node, &proot_addr, delete_root);
    __bt_addr_free_node(paddr_node);
  }

  // delete all src tree nodes:
  __tsrc_t *psrc_node;
  while (proot_src != NULL) {
    psrc_node = *(__tsrc_t**)proot_src;
    tdelete((void *)psrc_node, &proot_src, delete_root);
    __bt_src_free_node(psrc_node);
  }

  size_t btree_cur = __get_btree_bytes_current();
  size_t btree_max = __get_btree_bytes_max();
  const char *btleak = (btree_cur != 0) ? "(LEAK)" : "";
  __memprintfx("BTREE%s unfreed64=%zu, max64=%zu\n", btleak, btree_cur,
               btree_max);
}
}  // namespace


void *__malloc_trace(size_t size, const char *pfname, int linenum) {
  if (size == 0u)
    size = 1u;
  void *p = __malloc_orig(size);
  int sverrno = errno;
  if (__isZoslibInitialized() && !__doLogMemoryUsage())
    return p;

  if (p == NULL) {
    __memprintf("ERROR malloc failed, errno=%d size=%zu " \
                "(heap=%zu max=%zu) (%s:%d)\n", sverrno, size,
                curheap64, maxheap64, __file_basename(pfname), linenum);
    __display_backtrace(__getLogMemoryFileNo());
  } else {
    __taddr_t *pn;
    bool isCached = false;
    addptr(&pn, __MEMSPACE_64, p, size, pfname, linenum, &isCached);
    if (isCached) {
      __free_trace(p);
      p = __malloc_orig(size);
      if (p == NULL) {
        __memprintf("ERROR malloc failed, errno=%d size=%zu " \
                    "(heap=%zu max=%zu) (%s:%d)\n", sverrno, size,
                    curheap64, maxheap64, __file_basename(pfname), linenum);
        __display_backtrace(__getLogMemoryFileNo());
        return NULL;
      }
    }
    if (__doLogMemoryAll() || __doLogMemoryInc(curheap64, &lastvaldisp64)) {
      __memprintf("addr=%p size=%zu malloc OK (heap=%zu max=%zu) (%s-%d)\n",
                  p, size, curheap64, maxheap64, pn->psrc, pn->callnum);
    }
    if (doTraceback(pn->psrc)) {
      __display_backtrace(__getLogMemoryFileNo());
    }
  }
  return p;
}

void *__calloc_trace(size_t num, size_t size, const char *pfname, int linenum) {
  void *p = __calloc_orig(num, size);
  int sverrno = errno;
  if (__isZoslibInitialized() && !__doLogMemoryUsage())
    return p;

  if (p == NULL) {
    __memprintf("ERROR calloc failed, errno=%d nitems=%zu size=%zu total=%zu " \
                "(heap=%zu max=%zu) (%s:%d)\n", sverrno, num, size, num*size,
                curheap64, maxheap64, __file_basename(pfname), linenum);
    __display_backtrace(__getLogMemoryFileNo());
  } else {
    __taddr_t *pn;
    bool isCached = false;
    addptr(&pn, __MEMSPACE_64, p, num*size, pfname, linenum, &isCached);
    if (isCached) {
      __free_trace(p);
      p = __calloc_orig(num, size);
      if (p == NULL) {
        __memprintf("ERROR calloc failed, errno=%d nitems=%zu size=%zu " \
                    "total=%zu (heap=%zu max=%zu) (%s:%d)\n", sverrno, num,
                    size, num*size, curheap64, maxheap64,
                    __file_basename(pfname), linenum);
        __display_backtrace(__getLogMemoryFileNo());
        return NULL;
      }
    }
    if (__doLogMemoryAll() || __doLogMemoryInc(curheap64, &lastvaldisp64)) {
      __memprintf("addr=%p nitems=%zu size=%zu total=%zu calloc OK " \
                  "(heap=%zu max=%zu) (%s-%d)\n", p, num, size,
                  num*size, curheap64, maxheap64, pn->psrc, pn->callnum);
    }
    if (doTraceback(pn->psrc)) {
      __display_backtrace(__getLogMemoryFileNo());
    }
  }
  return p;
}

char *__strdup_trace(const char *ptr, const char *pfname, int linenum) {
  char *p = __strdup_orig(ptr);
  int sverrno = errno;
  if (__isZoslibInitialized() && !__doLogMemoryUsage())
    return p;

  size_t size = strlen(ptr);
  if (p == NULL) {
    __memprintf("ERROR strdup failed errno=%d size=%zu\n",
                "(heap=%zu max=%zu) (%s:%d)\n", sverrno, size,
                curheap64, maxheap64, __file_basename(pfname), linenum);
    __display_backtrace(__getLogMemoryFileNo());
  } else {
    __taddr_t *pn;
    bool isCached = false;
    addptr(&pn, __MEMSPACE_64, p, size, pfname, linenum, &isCached);
    if (isCached) {
      __free_trace(p);
      p = __strdup_orig(ptr);
      if (p == NULL) {
        __memprintf("ERROR strdup failed errno=%d size=%zu\n",
                    "(heap=%zu max=%zu) (%s:%d)\n", sverrno, size,
                    curheap64, maxheap64, __file_basename(pfname), linenum);
        __display_backtrace(__getLogMemoryFileNo());
        return NULL;
      }
    } 
    if (__doLogMemoryAll() || __doLogMemoryInc(curheap64, &lastvaldisp64)) {
      __memprintf("addr=%p size=%zu strdup OK (heap=%zu max=%zu) (%s-%d)\n",
                  p, size, curheap64, maxheap64, pn->psrc, pn->callnum);
    }
    if (doTraceback(pn->psrc)) {
      __display_backtrace(__getLogMemoryFileNo());
    }
  }
  return p;
}

char *__strndup_trace(const char *s, size_t n,
                      const char *pfname, int linenum) {
  size_t len = strnlen(s, n);
  char *dupStr = static_cast<char*>(__malloc_trace(len + 1, pfname, linenum));
  if (dupStr == NULL)
    return NULL;
  dupStr[len] = '\0';
  return static_cast<char*>(memcpy(dupStr, s, len));
}

void __free_trace(void *ptr) {
  if (ptr == NULL)
    return;
  if (__isZoslibInitialized() && !__doLogMemoryUsage()) {
    __free_orig(ptr);
    return;
  }
  size_t size = freeptr(ptr);
  __free_orig(ptr);
  if (__doLogMemoryAll()) {
    __memprintf("addr=%p size=%zu free OK (heap=%zu max=%zu)\n",
                ptr, size, curheap64, maxheap64);
  }
}

void *__realloc_trace(void *ptr, size_t new_size, const char *pfname, int linenum) {
  if (__isZoslibInitialized() && !__doLogMemoryUsage())
    return __realloc_orig(ptr, new_size);

  // If size is 0 and ptr is not NULL, the storage pointed to by ptr is freed
  // and NULL is returned.
  if (ptr == NULL && new_size == 0u) {
    if (__doLogMemoryWarning()) {
      __memprintf("WARN realloc called with ptr=0 new-size=0 " \
                  "(heap=%zu max=%zu) (%s:%d)\n",
                  curheap64, maxheap64, __file_basename(pfname), linenum);
      if (gDoWarningTB)
        __display_backtrace(__getLogMemoryFileNo());
    }
    return NULL;
  }
  void *newptr = NULL;
  if (new_size == 0u) {
    __free_trace(ptr);
  } else if (ptr != NULL) {
    newptr = __malloc_trace(new_size, pfname, linenum);
    int sverrno = errno;
    if (newptr != NULL) {
      size_t old_size = getsize(ptr, pfname, linenum);
      memcpy(newptr, ptr, std::min(new_size, old_size));
      __free_trace(ptr);
    } else {
      __free_trace(ptr);
      size_t old_size = getsize(ptr, pfname, linenum);
      __memprintf("ERROR realloc failed errno=%d ptr=%p, size=%zu " \
                  "new-size=%zu (heap=%zu max=%zu) (%s:%d)\n",
                  sverrno, ptr, old_size, new_size,
                  curheap64, maxheap64, __file_basename(pfname), linenum);
      __display_backtrace(__getLogMemoryFileNo());
    }
  } else {
    newptr = __malloc_trace(new_size, pfname, linenum);
  }
  return newptr;
}

void *__reallocf_trace(void *ptr, size_t new_size,
                       const char *pfname, int linenum) {
  void *newptr = __realloc_trace(ptr, new_size, pfname, linenum);
  if (newptr == NULL && new_size > 0)
    __free_trace(ptr);
  return newptr;
}

void *__malloc31_trace(size_t size, const char *pfname, int linenum) {
  void *p = __malloc31_orig(size);
  int sverrno = errno;
  if (__isZoslibInitialized() && !__doLogMemoryUsage())
    return p;

  if (p == NULL) {
    __memprintf("ERROR malloc31 failed, errno=%d size=%zu " \
                "(m31=%zu max=%zu) (%s:%d)\n", sverrno, size,
                curmem31, maxmem31, __file_basename(pfname), linenum);
    __display_backtrace(__getLogMemoryFileNo());
  } else {
    __taddr_t *pn;
    addptr(&pn, __MEMSPACE_31, p, size, pfname, linenum);
    if (__doLogMemoryAll() || __doLogMemoryInc(curmem31, &lastvaldisp31)) {
      __memprintf("addr=%p size=%zu malloc31 OK (m31=%zu max=%zu) (%s-%d)\n",
                  p, size, curmem31, maxmem31, pn->psrc, pn->callnum);
    }
    if (doTraceback(pn->psrc)) {
        __display_backtrace(__getLogMemoryFileNo());
    }
  }
  return p;
}

extern "C" void __display_alloc_stats(bool bDestroy, bool bDisplayAll) {
  // This is called from destruct() or can be called by the app (e.g. on every
  // SIGUSR2, in which case bDisplayAll can be passed as false to display only
  // changes in unfreed memory).
  if (bDisplayAll)
    gbDisplayAllAllocStats = true;
  display_stats();
#if defined(ZOSLIB_TRACE_ALLOCS)
  if (bDestroy)
    destroy_nodes();
#endif
  heapreport();
  if (bDestroy)
    pthread_mutex_destroy(&access_lock);
  gbDisplayAllAllocStats = false;
}

__attribute__((constructor)) void init_allocs() {
  char *penv = getenv("__MEMORY_USAGE_ALLOC_TB_WARNING");
  gDoWarningTB = (penv && *penv == '1');

  if (pthread_mutex_init(&access_lock, NULL) != 0) {
    perror("pthread_mutex_init");
    abort();
  }
}

__attribute__((destructor)) void destruct() {
  // This can only be called if the process terminated gracefully; generate
  // allocations report and cleanup.

  __display_alloc_stats(true, true);
}
#endif  // !ZOSLIB_TRACE_ALLOCS

// TODO(gabylb): move all IARV64 and 31-bit allocs from zos-base.h and zos.cc
// to separate files and move these declarations to the new .h:
extern "C" void *__iarv64_alloc(int segs, const char *token,
                                long long *prc, long long *preason);
extern "C" long long __iarv64_free(void *ptr, const char *token,
                                   long long *preason);

#ifndef ZOSLIB_TRACE_ALLOCS
extern "C" void *__alloc_seg(size_t segs) {
  long long rc, reason;
  return __iarv64_alloc(segs, gxttoken, &rc, &reason);
}
#else
extern "C" void *__alloc_seg_trace(size_t segs,
                                   const char *pfname, int linenum) {
  long long rc, reason;
  void *p = __iarv64_alloc(segs, gxttoken, &rc, &reason);
  int sverrno = errno;
  if (__isZoslibInitialized() && !__doLogMemoryUsage())
    return p;
  size_t size = segs * 1024u * 1024u;
  if (p == NULL) {
    __memprintf("ERROR __iarv64_alloc failed, rc=%llx reason=%llx " \
                " errno=%d size=%zu " \
                "(64v=%zu max=%zu) (%s:%d)\n", rc, reason, sverrno, size,
                curmem64v, maxmem64v, __file_basename(pfname), linenum);
    __display_backtrace(__getLogMemoryFileNo());
  } else {
    __taddr_t *pn;
    addptr(&pn, __MEMSPACE_64V, p, size, pfname, linenum);
    if (__doLogMemoryAll() || __doLogMemoryInc(curmem64v, &lastvaldisp64v)) {
      __memprintf("addr=%p size=%zu iarv64_alloc OK (v64=%zu max=%zu) (%s-%d)\n",
                  p, size, curmem64v, maxmem64v, pn->psrc, pn->callnum);
    }
    if (doTraceback(pn->psrc)) {
      __display_backtrace(__getLogMemoryFileNo());
    }
  }
  return p;
}
#endif

extern "C" int __free_seg(void *ptr, size_t reqsize) {
  if (ptr == NULL)
    return 0;
  long long rc, reason;
#ifndef ZOSLIB_TRACE_ALLOCS
  return __iarv64_free(ptr, gxttoken, &reason);
#else
  rc = __iarv64_free(ptr, gxttoken, &reason);
  int sverrno = errno;
  if (!__doLogMemoryUsage() && __isZoslibInitialized())
    return rc;

  size_t size = freeptr(ptr);
  if (rc) {
    __memprintf("ERROR __iarv64_free failed, rc=%llx reason=%llx " \
                "errno=%d size=%zu (64v=%zu max=%zu)\n",
                rc, reason, sverrno, size, curmem64v, maxmem64v);
    __display_backtrace(__getLogMemoryFileNo());
  } else if (__doLogMemoryAll()) {
    __memprintf("addr=%p size=%zu iarv4_free OK (heap=%zu max=%zu)\n",
                ptr, size, curmem64v, maxmem64v);
  }
  return rc;
#endif
}
