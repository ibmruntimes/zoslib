///////////////////////////////////////////////////////////////////////////////
// Licensed Materials - Property of IBM
// ZOSLIB
// (C) Copyright IBM Corp. 2026. All Rights Reserved.
// US Government Users Restricted Rights - Use, duplication
// or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
///////////////////////////////////////////////////////////////////////////////

// TODO: revisit when `thread_local` is fully implemented on z/OS.

/*
 * Thread-local storage for z/OS (C++11).
 *
 * The latest LLVM based compilers on z/OS (such as Open XL C/C++ 2.2) still
 * do not fully support the C++ `thread_local` keyword: only trivially
 * destructible variables.
 *
 * The existing simulation `__tlssim` doesn't work with such variables either.
 * Also, it doesn't support initialization per thread.
 *
 * __tls<T> addresses the issues assuming it's used to port C++11 code (or
 * higher level) in the most seamless way possible.
 *
 *
 * Usage examples:
 *
 *   static __tls<int> tls_counter;
 *   auto &counter = tls_counter.get([]{ return 0; });
 *   counter++;
 *
 *   static __tls<std::string> tls_name;
 *   auto &name = tls_name.get([]{ return std::string("default"); });
 *   name = "updated";
 */

#ifndef ZOS_TLS_CXX_H_
#define ZOS_TLS_CXX_H_

#if !defined(__cplusplus) || __cplusplus < 201103L
#error "zos-tls-cxx.h requires C++11 or later"
#endif

#include "zos-macros.h"

#include <atomic>
#include <mutex>
#include <pthread.h>
#include <system_error>
#include <utility>

template <typename T> class __Z_EXPORT __tls {

  std::once_flag once;
  pthread_key_t key;
  std::atomic<bool> key_created;

  /// Destructor callback registered with pthread_key_create.
  static void destroy(void *ptr) noexcept { delete static_cast<T *>(ptr); }

  /// Create the pthread key exactly once
  void ensure_key() {
    std::call_once(once, [this]() {
      const int rc = pthread_key_create(&key, &destroy);
      if (rc != 0)
        throw std::system_error(rc, std::generic_category(),
                                "pthread_key_create");
      key_created.store(true, std::memory_order_release);
    });
  }

  /// Get raw per-thread pointer (nullptr if not set yet).
  T *raw() const noexcept { return static_cast<T *>(pthread_getspecific(key)); }

  /// Install a newly-allocated T*
  T &install(T *p) {
    const int rc = pthread_setspecific(key, p);
    if (rc != 0) {
      delete p;
      throw std::system_error(rc, std::generic_category(),
                              "pthread_setspecific");
    }
    return *p;
  }

public:
  __tls() noexcept : key_created(false) {}

  // It shouldn't be copyable or movable.
  __tls(const __tls &) = delete;
  __tls &operator=(const __tls &) = delete;
  __tls(__tls &&) = delete;
  __tls &operator=(__tls &&) = delete;

  /// Note: it's expected to be static
  ~__tls() {
    if (key_created.load(std::memory_order_acquire)) {
      T *p = raw();
      if (p) {
        delete p;
        pthread_setspecific(key, nullptr);
      }
      pthread_key_delete(key);
    }
  }

  /// Get a reference to the calling thread's T, default-constructing it
  /// on first access.
  T &get() {
    ensure_key();
    T *p = raw();
    return p ? *p : install(new T());
  }

  /// Get a reference to the calling thread's T, initializing it with init() on
  /// first access.
  template <typename Init> T &get(Init &&init) {
    ensure_key();
    T *p = raw();
    return p ? *p : install(new T(std::forward<Init>(init)()));
  }
};

#endif // ZOS_TLS_CXX_H_
