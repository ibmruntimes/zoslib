///////////////////////////////////////////////////////////////////////////////
// Licensed Materials - Property of IBM
// ZOSLIB
// (C) Copyright IBM Corp. 2025. All Rights Reserved.
// US Government Users Restricted Rights - Use, duplication
// or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
///////////////////////////////////////////////////////////////////////////////

#if defined(__clang__) && !defined(__ibmxl__) && defined(__cpp_aligned_new) && \
    defined(ZOSLIB_ALIGNED_NEWDEL) && !_LIBCPP_HAS_ALIGNED_ALLOCATION

// __cpp_aligned_new is defined with -faligned-allocation which is required to
// compile this test.

#include "zos.h"
#include "gtest/gtest.h"

#include <new>

namespace {

bool bHandlerCalled;

size_t getPhysMemory() {
  size_t pages = __get_num_frames();
  size_t page_size = sysconf(_SC_PAGESIZE);
  return pages * page_size;
}

void handler()
{
  std::set_new_handler(nullptr);
  bHandlerCalled = true;
}

TEST(AlignedNewDel, TestBadAllocArrHandler) {
  size_t alignment = 8;
  size_t maxmem = getPhysMemory();
  std::set_new_handler(handler);
  auto ptr = new(std::align_val_t(alignment), (std::nothrow)) int[maxmem];
  ASSERT_EQ(bHandlerCalled, true);
  ASSERT_EQ(ptr, nullptr);
}

#if __EXCEPTIONS
TEST(AlignedNewDel, TestBadAllocArr) {
  size_t alignment = 8;
  bool gotex = false;
  try {
    size_t maxmem = getPhysMemory();
    auto ptr = new(std::align_val_t(alignment)) int[maxmem];
    // This is so compiler doesn't optimize out the above call:
    ASSERT_EQ(ptr, nullptr);
  } catch (const std::bad_alloc& e) {
    gotex = true;
  }
  ASSERT_EQ(gotex, true);
}
#endif

TEST(AlignedNewDel, TestBadAllocArrNoThrow) {
  size_t alignment = 8;
  size_t maxmem = getPhysMemory();
  auto ptr = new(std::align_val_t(alignment), (std::nothrow)) int[maxmem];
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

TEST(AlignedNewDel, TestAllocArrNoThrowDelSize) {
  size_t alignment = sysconf(_SC_PAGESIZE);
  auto ptr = new(std::align_val_t(alignment), (std::nothrow)) int[123];
  ASSERT_NE(ptr, nullptr);
  ASSERT_EQ(reinterpret_cast<size_t>(ptr) % alignment, 0);
  ::operator delete[](ptr, sizeof(int[123]), std::align_val_t(alignment));
}

TEST(AlignedNewDel, TestAllocNoThrowDelSize) {
  size_t alignment = sysconf(_SC_PAGESIZE);
  auto ptr = new(std::align_val_t(alignment)) int;
  ASSERT_NE(ptr, nullptr);
  ASSERT_EQ(reinterpret_cast<size_t>(ptr) % alignment, 0);
  ::operator delete(ptr, sizeof(int), std::align_val_t(alignment));
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
