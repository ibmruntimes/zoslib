///////////////////////////////////////////////////////////////////////////////
// Licensed Materials - Property of IBM
// ZOSLIB
// (C) Copyright IBM Corp. 2025. All Rights Reserved.
// US Government Users Restricted Rights - Use, duplication
// or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
///////////////////////////////////////////////////////////////////////////////

#if defined(ZOSLIB_ALIGNED_NEWDEL) && defined(__cpp_aligned_new)

// __cpp_aligned_new is defined with -faligned-allocation which is required
// if you supply your own aligned allocation functions, as is the case here.

#include "zos.h"
#include "gtest/gtest.h"

#include <new>
#include <sys/resource.h>

namespace {

bool bHandlerCalled;

size_t getMemlimit() {
  struct rlimit lim;
  if (getrlimit(RLIMIT_MEMLIMIT, &lim) == -1) {
    perror("setrlimit");
    exit(-1);
  }
  return lim.rlim_cur;
}

#if __EXCEPTIONS

void handler()
{
  std::set_new_handler(nullptr);
  bHandlerCalled = true;
}

TEST(AlignedNewDel, TestBadAllocArrHandler) {
  size_t alignment = 8;
  std::set_new_handler(handler);
  size_t maxelems = getMemlimit() / sizeof(int);
  auto ptr = new(std::align_val_t(alignment), (std::nothrow)) int[maxelems+1];
  ASSERT_EQ(bHandlerCalled, true);
  ASSERT_EQ(ptr, nullptr);
}


TEST(AlignedNewDel, TestBadAllocArr) {
  size_t alignment = 8;
  bool gotex = false;
  try {
    size_t maxelems = getMemlimit() / sizeof(int);
    auto ptr = new(std::align_val_t(alignment)) int[maxelems+1];
    // This is so compiler doesn't optimize out the above call:
    ASSERT_EQ(ptr, nullptr);
  } catch (const std::bad_alloc& e) {
    gotex = true;
  }
  ASSERT_EQ(gotex, true);
}

#endif // __EXCEPTIONS

TEST(AlignedNewDel, TestBadAllocArrNoThrow) {
  size_t alignment = 8;
  size_t maxelems = getMemlimit() / sizeof(int);
  auto ptr = new(std::align_val_t(alignment), (std::nothrow)) int[maxelems+1];
  ASSERT_EQ(ptr, nullptr);
}

TEST(AlignedNewDel, TestAlloc) {
  size_t alignment = sysconf(_SC_PAGESIZE);
  auto ptr = new(std::align_val_t(alignment)) int;
  ASSERT_NE(ptr, nullptr);
  ASSERT_EQ(reinterpret_cast<size_t>(ptr) % alignment, 0);
  ::operator delete(ptr, std::align_val_t(alignment));
}

TEST(AlignedNewDel, TestAllocNoThrow) {
  size_t alignment = sysconf(_SC_PAGESIZE);
  auto ptr = new(std::align_val_t(alignment), (std::nothrow)) int;
  ASSERT_NE(ptr, nullptr);
  ASSERT_EQ(reinterpret_cast<size_t>(ptr) % alignment, 0);
  ::operator delete(ptr, std::align_val_t(alignment), (std::nothrow));
}

TEST(AlignedNewDel, TestAllocArrNoThrow) {
  size_t alignment = sysconf(_SC_PAGESIZE);
  auto ptr = new(std::align_val_t(alignment), (std::nothrow)) int[123];
  ASSERT_NE(ptr, nullptr);
  ASSERT_EQ(reinterpret_cast<size_t>(ptr) % alignment, 0);
  ::operator delete[](ptr, std::align_val_t(alignment), (std::nothrow));
}

TEST(AlignedNewDel, TestAllocArr) {
  size_t alignment = sysconf(_SC_PAGESIZE);
  auto ptr = new(std::align_val_t(alignment)) int[123];
  ASSERT_NE(ptr, nullptr);
  ASSERT_EQ(reinterpret_cast<size_t>(ptr) % alignment, 0);
  ::operator delete[](ptr, std::align_val_t(alignment));
}

}      // namespace
#endif
