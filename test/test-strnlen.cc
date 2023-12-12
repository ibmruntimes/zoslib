//#include "zos.h"
#include "gtest/gtest.h"

namespace {

TEST(StrnlenTest, StrLtLimit) {
  char *teststr = (char *) malloc (102);
  memset (teststr, 'A', 102);
  teststr[100] = '\0';
  int teststrlen = strnlen(teststr, 101);
  EXPECT_EQ(teststrlen, 100);
}

TEST(StrnlenTest, StrEqLimit) {
  char *teststr = (char *) malloc (102);
  memset (teststr, 'A', 102);
  teststr[100] = '\0';
  int teststrlen = strnlen(teststr, 100);
  EXPECT_EQ(teststrlen, 100);
}

TEST(StrnlenTest, StrGtLimit) {
  char *teststr = (char *) malloc (102);
  memset (teststr, 'A', 102);
  teststr[100] = '\0';
  int teststrlen = strnlen(teststr, 99);
  EXPECT_EQ(teststrlen, 99);
}

TEST(StrnlenTest, BigStrLtLimit) {
  char *teststr = (char *) malloc (100002);
  memset (teststr, 'A', 100002);
  teststr[100000] = '\0';
  int teststrlen = strnlen(teststr, 100001);
  EXPECT_EQ(teststrlen, 100000);
}

TEST(StrnlenTest, BigStrEqLimit) {
  char *teststr = (char *) malloc (100002);
  memset (teststr, 'A', 100002);
  teststr[100000] = '\0';
  int teststrlen = strnlen(teststr, 100000);
  EXPECT_EQ(teststrlen, 100000);
}

TEST(StrnlenTest, BigStrGtLimit) {
  char *teststr = (char *) malloc (100002);
  memset (teststr, 'A', 100002);
  teststr[100000] = '\0';
  int teststrlen = strnlen(teststr, 99999);
  EXPECT_EQ(teststrlen, 99999);
}
} // namespace

