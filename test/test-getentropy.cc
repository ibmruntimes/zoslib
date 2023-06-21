#include <unistd.h>
#include "gtest/gtest.h"

namespace {

TEST(GetentropyTest, Getentropy256) {
  char buf[256];
  memset(buf, 0, sizeof(buf));

  int rc = getentropy(buf, sizeof(buf));
  EXPECT_EQ(rc, 0);

  bool buffer_is_null = true;
  for (int i = 0; i < sizeof(buf); i++) {
    if (buf[i] != 0) {
      buffer_is_null = false;
      break;
    }
  }
  EXPECT_FALSE(buffer_is_null);
}

TEST(GetentropyTest, Getentropy257) {
  char buf[257];
  memset(buf, 0, sizeof(buf));

  int rc = getentropy(buf, sizeof(buf));
  EXPECT_EQ(rc, 0);

  bool buffer_is_null = true;
  for (int i = 0; i < sizeof(buf); i++) {
    if (buf[i] != 0) {
      buffer_is_null = false;
      break;
    }
  }
  EXPECT_FALSE(buffer_is_null);
}

TEST(GetentropyTest, Getentropy258) {
  char buf[258];
  int rc = getentropy(buf, sizeof(buf));
  EXPECT_EQ(rc, -1);
  EXPECT_EQ(errno, EIO);
}

} // namespace
