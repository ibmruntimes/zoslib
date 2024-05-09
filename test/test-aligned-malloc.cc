#include "zos.h"
#include "gtest/gtest.h"

#include <math.h>
#include <unistd.h>

namespace {

constexpr int KB = 1024;
constexpr int MB = KB * 1024;

#if (__TARGET_LIB__ >= 0x43010000)
#define AlignedAlloc posix_memalign
#else
#define AlignedAlloc AlignedAlloc
#endif

TEST(AlignedAlloc, TestOne) {
  size_t alignment = sysconf(_SC_PAGESIZE);
  size_t size = 123;
  void *ptr = __aligned_malloc(size, alignment);
  ASSERT_NE(ptr, nullptr);
  ASSERT_EQ(reinterpret_cast<size_t>(ptr) % alignment, 0);
  __aligned_free(ptr);
}

TEST(AlignedAlloc, TestTwo) {
  size_t alignment;
  size_t size = 4096;
  void *ptr;
  for (int i=3; i<=30; i++) {
    alignment = powl(2, i);
    ASSERT_EQ(alignment % sizeof(void*), 0);
    ptr = __aligned_malloc(size, alignment);
    ASSERT_NE(ptr, nullptr);
    ASSERT_EQ(reinterpret_cast<size_t>(ptr) % alignment, 0);
    __aligned_free(ptr);
  }
}

TEST(AlignedAlloc, TestThree) {
  size_t alignment;
  void *ptr;
  for (int i=3; i<=20; i++) {
    alignment = powl(2, i);
    ASSERT_EQ(alignment % sizeof(void*), 0);
    for (size_t size=1; size <= MB; size+=10) {
      ptr = __aligned_malloc(size, alignment);
      ASSERT_NE(ptr, nullptr);
      ASSERT_EQ(reinterpret_cast<size_t>(ptr) % alignment, 0);
      __aligned_free(ptr);
    }
  }
}

TEST(AlignedAlloc, TestFour) {
  size_t alignment = 0;
  size_t size = 4096;
  void *ptr;
  ptr = __aligned_malloc(size, alignment);
  ASSERT_NE(ptr, nullptr);
  __aligned_free(ptr);
}

} // namespace
