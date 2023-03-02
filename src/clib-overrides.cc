///////////////////////////////////////////////////////////////////////////////
// Licensed Materials - Property of IBM
// ZOSLIB
// (C) Copyright IBM Corp. 2023. All Rights Reserved.
// US Government Users Restricted Rights - Use, duplication
// or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
///////////////////////////////////////////////////////////////////////////////

#define _AE_BIMODAL 1
#include "zos-base.h"

#include <assert.h>

#ifdef __cplusplus
#include <algorithm>
#include <unordered_map>
extern "C" {
#endif

#ifdef ZOSLIB_OVERRIDE_CLIB_ALLOCS
#include <string.h>

typedef unsigned long value_type;
typedef unsigned long key_type;
bool __isZoslibInitialized();

struct __hash_func {
  size_t operator()(const key_type &k) const {
    int s = 0;
    key_type n = k;
    while (0 == (n & 1) && s < (sizeof(key_type) - 1)) {
      n = n >> 1;
      ++s;
    }
    return s + (n * 0x744dcf5364d7d667UL);
  }
};

typedef std::unordered_map<key_type, value_type, __hash_func>::const_iterator
    mem_cursor_t;

class __HeapCache {
  std::unordered_map<key_type, value_type, __hash_func> cache;
  std::mutex access_lock;
  size_t curheap;
  size_t maxheap;

public:
  __HeapCache() {
    curheap = 0u;
    maxheap = 0u;
  }
  size_t getCurrentHeap() { return curheap; }
  size_t getMaxHeap() { return maxheap; }

  void addptr(const void *ptr, size_t v) {
    unsigned long k = (unsigned long)ptr;
    std::lock_guard<std::mutex> guard(access_lock);
    if (__doLogMemoryWarning()) {
      mem_cursor_t c = cache.find(k);
      if (c != cache.end()) {
        __memprintf("HWARN haddr=%p already in cache with " \
                    "size=%zu; new-size=%zu\n",
                    ptr, c->second, v);
      }
    }
    cache[k] = v;
    curheap += v;
    maxheap = std::max(maxheap, curheap);
  }
  size_t freeptr(const void *ptr) {
    unsigned long k = (unsigned long)ptr;
    std::lock_guard<std::mutex> guard(access_lock);
    mem_cursor_t c = cache.find(k);
    if (c != cache.end()) {
      size_t size = c->second;
      curheap -= size;
      cache.erase(c);
      return size;
    } else if (__doLogMemoryWarning()) {
      __memprintf("HWARN free haddr=%p not in " \
                  "cache, returning size of 0\n", ptr);
    }
    return 0u;
  }
  void displayDebris() {
    if (__doLogMemoryWarning()) {
      for (mem_cursor_t it = cache.begin(); it != cache.end(); ++it) {
        __memprintf("HDEBRIS haddr=%lX size=%lu\n",
                    it->first, it->second);
      }
    }
  }
  ~__HeapCache() {
    // This shouldn't be called.
    assert(0);
  }
};

// This is static so it can be used by [de]allocation calls during termination:
static __HeapCache* __galloc_info = nullptr;

static __HeapCache * __get_galloc_info() {
  assert(__galloc_info != nullptr);
  return __galloc_info;
}

void __init_heap_cache() {
  std::mutex access_lock;
  std::lock_guard<std::mutex> guard(access_lock);
  if (__galloc_info == nullptr) {
    __galloc_info = new __HeapCache;
  }
}

void __displayHeapDebris() {
  __get_galloc_info()->displayDebris();
}

void *__calloc_trace(size_t num, size_t size) {
  // This is called on every alloc function because it can be called
  // before other zoslib static variables (that would otherwise do the
  // init) are constructed; e.g. SigintWatchdogHelper constructor.
  // __isZoslibInitialized() is called here for the same reason.
  __init_heap_cache();
  if (__isZoslibInitialized() && !__doLogMemoryUsage()) {
    return __calloc_orig(num, size);
  }
  void *p = __calloc_orig(num, size);
  if (!p) {
    __memprintf("HERROR calloc failed, errno=%d " \
                "nitems=%zu size=%zu total=%zu " \
                "(heap=%zu max=%zu)\n",
                errno, num, size, num*size,
                __get_galloc_info()->getCurrentHeap(),
                __get_galloc_info()->getMaxHeap());
  } else {
    __get_galloc_info()->addptr(p, num*size);
    if (__doLogMemoryAll()) {
      __memprintf("haddr=%p nitems=%zu size=%zu total=%zu calloc OK " \
                  "(heap=%zu max=%zu)\n",
                  p, num, size, num*size, __get_galloc_info()->getCurrentHeap(),
                  __get_galloc_info()->getMaxHeap());
    }
  }
  return p;
}

void *__malloc_trace(size_t size) {
  __init_heap_cache();
  if (__isZoslibInitialized() && !__doLogMemoryUsage()) {
    return __malloc_orig(size);
  }
  void *p = __malloc_orig(size);
  if (!p) {
    __memprintf("HERROR malloc failed, errno=%d size=%zu " \
                "(heap=%zu max=%zu)\n", errno, size,
                __get_galloc_info()->getCurrentHeap(),
                __get_galloc_info()->getMaxHeap());
  } else {
    __get_galloc_info()->addptr(p, size);
    if (__doLogMemoryAll()) {
      __memprintf("haddr=%p size=%zu malloc OK (heap=%zu max=%zu)\n",
                  p, size, __get_galloc_info()->getCurrentHeap(),
                  __get_galloc_info()->getMaxHeap());
    }
  }
  return p;
}

char *__strdup_trace(const char *ptr) {
  __init_heap_cache();
  if (__isZoslibInitialized() && !__doLogMemoryUsage()) {
    return __strdup_orig(ptr);
  }
  size_t size = strlen(ptr);
  char *p = __strdup_orig(ptr);
  if (!p) {
    __memprintf("HERROR strdup failed errno=%d size=%zu\n",
                "(heap=%zu max=%zu)\n", errno, size,
                __get_galloc_info()->getCurrentHeap(),
                __get_galloc_info()->getMaxHeap());
  } else {
    __get_galloc_info()->addptr(p, size);
    if (__doLogMemoryAll()) {
      __memprintf("haddr=%p size=%zu strdup OK (heap=%zu max=%zu)\n",
                  p, size, __get_galloc_info()->getCurrentHeap(),
                  __get_galloc_info()->getMaxHeap());
    }
  }
  return p;
}

void *__realloc_trace(void *ptr, size_t new_size) {
  __init_heap_cache();
  if (__isZoslibInitialized() && !__doLogMemoryUsage()) {
    return __realloc_orig(ptr, new_size);
  }
  // If size is 0 and ptr is not NULL, the storage pointed to by ptr is freed
  // and NULL is returned.
  if (ptr == nullptr && new_size == 0u) {
    if (__doLogMemoryWarning()) {
      __memprintf("HWARN realloc called with ptr=0 new-size=0 " \
                  "(heap=%zu max=%zu)\n",
                  __get_galloc_info()->getCurrentHeap(),
                  __get_galloc_info()->getMaxHeap());
    }
    return nullptr;
  }
  void *p = __realloc_orig(ptr, new_size);
  if (p == nullptr) {
    if (new_size > 0u) {
      __memprintf("HERROR realloc failed errno=%d size=%zu " \
                  "(heap=%zu max=%zu)\n", errno, new_size,
                  __get_galloc_info()->getCurrentHeap(),
                  __get_galloc_info()->getMaxHeap());
    } else {
      __get_galloc_info()->freeptr(ptr);
    }
  } else if (ptr != nullptr) {
    if (new_size > 0u) {
      size_t old_size = __get_galloc_info()->freeptr(ptr);
      __get_galloc_info()->addptr(p, new_size);
      if (__doLogMemoryAll()) {
        __memprintf("haddr=%p size=%zu old-ptr=%p old-size=%zu " \
                   "realloc OK (heap=%zu max=%zu)\n",
                    p, new_size, ptr, old_size,
                    __get_galloc_info()->getCurrentHeap(),
                    __get_galloc_info()->getMaxHeap());
      }
    } else {
      __memprintf("HERROR realloc called with ptr=%p and new-size=0, " \
                  "but returned %p instead of 0 (heap=%zu max=%zu)\n",
                  ptr, p, __get_galloc_info()->getCurrentHeap(),
                  __get_galloc_info()->getMaxHeap());
    }
  } else {
    __get_galloc_info()->addptr(p, new_size);
    if (__doLogMemoryAll()) {
      __memprintf("haddr=%p size=%zu old-ptr=0 realloc OK "\
                  "(heap=%zu max=%zu)\n",
                  p, new_size, __get_galloc_info()->getCurrentHeap(),
                  __get_galloc_info()->getMaxHeap());
    }
  }
  return p;
}

void __free_trace(void *ptr) {
  if (ptr == nullptr)
    return;
  __init_heap_cache();
  if (__isZoslibInitialized() && !__doLogMemoryUsage()) {
    __free_orig(ptr);
    return;
  }
  __free_orig(ptr);
  size_t size = __get_galloc_info()->freeptr(ptr);
  if (__doLogMemoryAll()) {
    __memprintf("haddr=%p size=%zu free OK (heap=%zu max=%zu)\n",
                ptr, size, __get_galloc_info()->getCurrentHeap(),
                __get_galloc_info()->getMaxHeap());
  }
}

#endif // ZOSLIB_OVERRIDE_CLIB_ALLOCS

#ifdef __cplusplus
}
#endif
