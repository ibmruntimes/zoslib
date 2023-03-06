#include "zos-getentropy.h"
#include "gtest/gtest.h"

namespace {

TEST(GetentropyTest, Getentropy256) {
  char buf[256];
  int rc = getentropy(buf, sizeof(buf));
  EXPECT_EQ(rc, 0);
  EXPECT_NE(((char*)buf)[0], 0);
}

TEST(GetentropyTest, Getentropy257) {
  char buf[257];
  int rc = getentropy(buf, sizeof(buf));
  EXPECT_EQ(rc, 0);
  EXPECT_NE(((char*)buf)[0], 0);
}

TEST(GetentropyTest, Getentropy258) {
  char buf[258];
  int rc = getentropy(buf, sizeof(buf));
  EXPECT_EQ(rc, -1);
  EXPECT_EQ(errno, EIO);
}

} // namespace
