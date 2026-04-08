///////////////////////////////////////////////////////////////////////////////
// Licensed Materials - Property of IBM
// ZOSLIB
// (C) Copyright IBM Corp. 2026. All Rights Reserved.
// US Government Users Restricted Rights - Use, duplication
// or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
///////////////////////////////////////////////////////////////////////////////

// Tests for __tls<T> â C++11 thread-local storage for z/OS.

#if defined(__cplusplus) && __cplusplus >= 201103L

#include "zos-tls-cxx.h"
#include "gtest/gtest.h"

#include <atomic>
#include <pthread.h>

namespace {

/// Run a callable on a new thread and join it.
template <typename Fn>
void run_on_thread(Fn &&fn) {
  struct Ctx {
    Fn *fn;
  };
  Ctx ctx{&fn};

  pthread_t tid;
  int rc = pthread_create(
      &tid, nullptr,
      +[](void *arg) -> void * {
        auto *c = static_cast<Ctx *>(arg);
        (*c->fn)();
        return nullptr;
      },
      &ctx);
  ASSERT_EQ(rc, 0) << "pthread_create failed";
  rc = pthread_join(tid, nullptr);
  ASSERT_EQ(rc, 0) << "pthread_join failed";
}

/// Track construction / destruction for leak detection.
struct Tracked {
  static std::atomic<int> counter;
  int value;

  explicit Tracked(int v = 0) : value(v) { counter.fetch_add(1); }

  Tracked(Tracked &&o) noexcept : value(o.value) {
    o.value = -1;
    counter.fetch_add(1);
  }

  Tracked(const Tracked &) = delete;
  Tracked &operator=(const Tracked &) = delete;
  Tracked &operator=(Tracked &&) = delete;

  ~Tracked() { counter.fetch_sub(1); }
};

std::atomic<int> Tracked::counter{0};


TEST(TlsTest, DefaultConstruction) {
  static __tls<int> tls;
  auto &v = tls.get();
  EXPECT_EQ(v, 0);
}

TEST(TlsTest, DefaultConstructionString) {
  static __tls<std::string> tls;
  auto &v = tls.get();
  EXPECT_TRUE(v.empty());
}

TEST(TlsTest, CallableInit) {
  static __tls<int> tls;
  auto &v = tls.get([] { return 42; });
  EXPECT_EQ(v, 42);
}

TEST(TlsTest, CallableInitString) {
  static __tls<std::string> tls;
  auto &v = tls.get([] { return std::string("42"); });
  EXPECT_EQ(v, "42");
}

TEST(TlsTest, CallableInvokedOnce) {
  static __tls<int> tls;
  int call_count = 0;
  auto init = [&call_count] {
    ++call_count;
    return 7;
  };

  auto &v1 = tls.get(init);
  auto &v2 = tls.get(init);
  auto &v3 = tls.get(init);

  EXPECT_EQ(call_count, 1);
  EXPECT_EQ(v1, 7);
  EXPECT_EQ(&v1, &v2);
  EXPECT_EQ(&v2, &v3);
}

TEST(TlsTest, AssignmentThroughReference) {
  static __tls<int> tls;
  auto &v = tls.get([] { return 1; });
  EXPECT_EQ(v, 1);

  v = 42;
  EXPECT_EQ(v, 42);

  auto &v2 = tls.get([] { return 1; });
  EXPECT_EQ(v2, 42);
  EXPECT_EQ(&v, &v2);
}

TEST(TlsTest, AssignmentString) {
  static __tls<std::string> tls;
  auto &v = tls.get([] { return std::string("initial"); });
  EXPECT_EQ(v, "initial");

  v = "updated";
  EXPECT_EQ(v, "updated");

  v += "!";
  EXPECT_EQ(tls.get([] { return std::string("ignored"); }), "updated!");
}

TEST(TlsTest, ThreadIsolation) {
  static __tls<int> tls;
  auto &main_val = tls.get([] { return 100; });
  main_val = 100;

  std::atomic<int> child_observed{-1};

  run_on_thread([&] {
    auto &child_val = tls.get([] { return 200; });
    child_observed.store(child_val);
    child_val = 300;
  });

  EXPECT_EQ(child_observed.load(), 200);
  EXPECT_EQ(main_val, 100);
}

TEST(TlsTest, ThreadsIndependent) {
  static __tls<int> tls;

  constexpr int threads_num = 8;
  std::atomic<int> results[threads_num];
  for (int i = 0; i < threads_num; ++i) {
    results[i].store(-1);
  }

  pthread_t threads[threads_num];
  struct Args {
    int id;
    std::atomic<int> *result;
  };
  Args args[threads_num];

  for (int i = 0; i < threads_num; ++i) {
    args[i] = {i, &results[i]};
    pthread_create(
        &threads[i], nullptr,
        +[](void *arg) -> void * {
          auto *a = static_cast<Args *>(arg);
          int id = a->id;
          auto &v = tls.get([id] { return id * 10; });
          v += 1;
          a->result->store(v);
          return nullptr;
        },
        &args[i]);
  }

  for (int i = 0; i < threads_num; ++i)
    pthread_join(threads[i], nullptr);

  for (int i = 0; i < threads_num; ++i)
    EXPECT_EQ(results[i].load(), i * 10 + 1) << "thread " << i;
}

TEST(TlsTest, DestructorOnThreadExit) {
  int before = Tracked::counter.load();

  {
    static __tls<Tracked> tls;
    run_on_thread([&] {
      tls.get([] { return Tracked(42); });
    });
  }

  EXPECT_EQ(Tracked::counter.load(), before);
}

TEST(TlsTest, DestructorOnAnchorDestruction) {
  int before = Tracked::counter.load();

  {
    __tls<Tracked> tls;
    tls.get([] { return Tracked(99); });
    EXPECT_EQ(Tracked::counter.load(), before + 1);
  }

  EXPECT_EQ(Tracked::counter.load(), before);
}

TEST(TlsTest, Vector) {
  static __tls<std::vector<int>> tls;
  auto &vec = tls.get([] { return std::vector<int>{1, 2, 3}; });
  ASSERT_EQ(vec.size(), 3u);
  EXPECT_EQ(vec[0], 1);

  vec.push_back(4);
  EXPECT_EQ(vec.size(), 4u);

  auto &vec2 = tls.get([] { return std::vector<int>{}; });
  EXPECT_EQ(vec2.size(), 4u);
  EXPECT_EQ(&vec, &vec2);
}

TEST(TlsTest, UniquePtr) {
  const int before = Tracked::counter.load();

  {
    __tls<std::unique_ptr<Tracked>> tls;
    auto &ptr = tls.get([] {
      return std::unique_ptr<Tracked>(new Tracked(123));
    });

    ASSERT_NE(ptr, nullptr);
    EXPECT_EQ(ptr->value, 123);
    EXPECT_EQ(Tracked::counter.load(), before + 1);

    ptr = std::unique_ptr<Tracked>(new Tracked(456));
    EXPECT_EQ(ptr->value, 456);
    EXPECT_EQ(Tracked::counter.load(), before + 1);
  }

  EXPECT_EQ(Tracked::counter.load(), before);
}

TEST(TlsTest, StressManyThreads) {
  static __tls<Tracked> tls;
  const int before = Tracked::counter.load();

  constexpr int threads_num = 32;
  pthread_t threads[threads_num];

  for (int i = 0; i < threads_num; ++i) {
    pthread_create(
        &threads[i], nullptr,
        +[](void *) -> void * {
          auto &v = tls.get([] { return Tracked(1); });
          v.value += 1;
          return nullptr;
        },
        nullptr);
  }
  for (int i = 0; i < threads_num; ++i)
    pthread_join(threads[i], nullptr);

  EXPECT_EQ(Tracked::counter.load(), before);
}

TEST(TlsTest, DefaultGet) {
  __tls<std::string> tls;
  auto &v = tls.get();
  EXPECT_TRUE(v.empty());

  v = "value";
  EXPECT_EQ(tls.get(), "value");
}

TEST(TlsTest, MultipleAnchors) {
  static __tls<int> tls_a;
  static __tls<int> tls_b;

  auto &a = tls_a.get([] { return 1; });
  auto &b = tls_b.get([] { return 2; });

  EXPECT_EQ(a, 1);
  EXPECT_EQ(b, 2);
  EXPECT_NE(&a, &b);

  a = 10;
  EXPECT_EQ(a, 10);
  EXPECT_EQ(b, 2);
}

} // namespace

#endif // __cplusplus >= 201103L
