#include "zos.h"
#include "gtest/gtest.h"

namespace {

TEST(DynamicV2R5, clock_gettime) {
  struct timespec start, stop;
  double accum;
  EXPECT_EQ(clock_gettime( CLOCK_REALTIME, &start), 0);
  system("sleep 1");
  EXPECT_EQ(clock_gettime( CLOCK_REALTIME, &stop), 0);
  EXPECT_GT(stop.tv_sec, start.tv_sec);
  EXPECT_GT(stop.tv_nsec, start.tv_nsec);
}

class DynamicV2R5Temp : public ::testing::Test {
    virtual void SetUp() {
      temp_path = tmpnam(NULL);
      temp_fp = fopen(temp_path, "w");
    }

    virtual void TearDown() {
      fclose(temp_fp);
      remove(temp_path);
    }

protected:
    char* temp_path;
    FILE *temp_fp;
};


TEST_F(DynamicV2R5Temp, lutimes) {
  struct timeval t[]= { { 0, 0 } , { 0, 0 } };
  EXPECT_EQ(lutimes(temp_path, t), 0);
  struct stat buff;
  lstat(temp_path, &buff);
  EXPECT_EQ(buff.st_atime, 0);
  EXPECT_EQ(buff.st_mtime, 0);
}

TEST_F(DynamicV2R5Temp, futimes) {
  struct timeval t[]= { { 0, 0 } , { 0, 0 } };
  EXPECT_EQ(futimes(fileno(temp_fp), t), 0);
  struct stat buff;
  fstat(fileno(temp_fp), &buff);
  EXPECT_EQ(buff.st_atime, 0);
  EXPECT_EQ(buff.st_mtime, 0);
}

} // namespace
