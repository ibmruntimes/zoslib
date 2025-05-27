#ifdef ZOSLIB_TRACE_ALLOCS

#include "test-args.h"
#include "zos.h"
#include "gtest/gtest.h"

#include <stdlib.h>
#include <string.h>

const size_t kMB = 1024u * 1024u;

TEST(AllocsTest, CAllocs) {
  const int N = 100;
  void *ptrs[N*5];
  int i;

  for (i=0; i<N; i++)
    ptrs[i] = malloc(kMB + i);

  for (; i<N*2; i++)
    ptrs[i] = calloc(kMB + i, 4);

  for (; i<N*3; i++) {
    void *p = (i % 2) ? malloc(kMB) : NULL;
    ptrs[i] = realloc(p, kMB + i);
  }

  const char *str1 = "strdup test string";
  for (; i<N*4; i++) {
    ptrs[i] = strdup(str1);
    EXPECT_STREQ(static_cast<char*>(ptrs[i]), str1);
  }

  const char *str2 = "1234567890";
  for (; i<N*5; i++) {
    ptrs[i] = strndup(str2, 8);
    EXPECT_STREQ(static_cast<char*>(ptrs[i]), "12345678");
  }
  for (i=0; i<N*5; i++)
    free(ptrs[i]);
}

TEST(AllocsTest, Malloc31) {
  const int N = 100;
  void *ptrs[N];
  for (int i=0; i<N; i++)
    ptrs[i] = __malloc31(kMB + i);

  for (int i=0; i<N; i++)
    free(ptrs[i]);
}

TEST(AllocsTest, Alloc31) {
  const int N = 100;
  void *ptrs[N];
  for (int i=0; i<N; i++)
    ptrs[i] = __zalloc(kMB + i + 1, PAGE_SIZE);

  for (int i=0; i<N; i++)
    __zfree(ptrs[i], PAGE_SIZE);
}

TEST(AllocsTest, Alloc64V) {
  const int N = 100;
  void *ptrs[N];
  for (int i=0; i<N; i++)
    ptrs[i] = __zalloc(kMB, PAGE_SIZE);

  for (int i=0; i<N; i++)
    __zfree(ptrs[i], PAGE_SIZE);
}

#endif // ZOSLIB_TRACE_ALLOCS
