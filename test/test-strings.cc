#include "zos.h"
#include "string.h"
#include "gtest/gtest.h"
#include <libgen.h>

namespace {

TEST(StrndupTest, CheckStrndupFunctionality) {
  // Test copying partial string
  const char* original = "Hello, World!";
  size_t length = 5; 

  char* copied = strndup(original, length);

  ASSERT_NE(copied, nullptr);

  EXPECT_STREQ(copied, "Hello");

  free(copied);

  // Test copying the entire string
  size_t full_length = strlen(original);
  char* full_copied = strndup(original, full_length);

  ASSERT_NE(full_copied, nullptr);

  EXPECT_STREQ(full_copied, original);

  free(full_copied);

  // Test 0 length
  char* zero_length_copied = strndup(original, 0);
  EXPECT_STREQ(zero_length_copied, "");
  free(zero_length_copied);
}

TEST(StpcpyTest, CheckStpcpyFunctionality) {
  const char *src = "hello";
  char dest[10];
  char *ret = stpcpy(dest, src);
  EXPECT_STREQ(dest, "hello");
  EXPECT_EQ(ret, dest + strlen(src));
  EXPECT_EQ(*ret, '\0');

  const char *src2 = " world";
  char *ret2 = stpcpy(ret, src2);
  EXPECT_STREQ(dest, "hello world");
  EXPECT_EQ(ret2, dest + strlen("hello world"));
  EXPECT_EQ(*ret2, '\0');
}

} // namespace
