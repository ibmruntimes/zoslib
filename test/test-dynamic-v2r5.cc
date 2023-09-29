#include "zos.h"
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/inotify.h>
#include <sys/eventfd.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include "gtest/gtest.h"

namespace {

TEST(DynamicV2R5, clock_gettime) {
  struct timespec start, stop;
  double accum;
  EXPECT_EQ(clock_gettime( CLOCK_REALTIME, &start), 0);
  system("sleep 1");
  EXPECT_EQ(clock_gettime( CLOCK_REALTIME, &stop), 0);
  EXPECT_GT(stop.tv_sec, start.tv_sec);
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

#if defined(ZOSLIB_ENABLE_V2R5_FEATURES)
TEST_F(DynamicV2R5Temp, pipe2) {
  int fd[2];
  int ret = pipe2(fd, O_CLOEXEC | O_NONBLOCK);
  EXPECT_EQ(ret, 0);
  EXPECT_GT(fd[0], 2);
  EXPECT_GT(fd[1], 2);
  close(fd[0]);
  close(fd[1]);
}

#ifdef _OE_SOCKETS
TEST_F(DynamicV2R5Temp, accept4) {
  int ret = accept4(-1, 0, 0, SOCK_NONBLOCK);
  EXPECT_LT(ret, 0);
}
#endif
#endif

TEST_F(DynamicV2R5Temp, futimes) {
  struct timeval t[]= { { 0, 0 } , { 0, 0 } };
  EXPECT_EQ(futimes(fileno(temp_fp), t), 0);
  struct stat buff;
  fstat(fileno(temp_fp), &buff);
  EXPECT_EQ(buff.st_atime, 0);
  EXPECT_EQ(buff.st_mtime, 0);
}

#if defined(ZOSLIB_ENABLE_V2R5_FEATURES)
TEST_F(DynamicV2R5Temp, inotify) {
  if (inotify_init) {
    inotify_init(); // Should not abort
  }
}

TEST_F(DynamicV2R5Temp, epoll) {
  if (epoll_create) {
    epoll_create(1); // Should not abort
  }
}

TEST_F(DynamicV2R5Temp, eventfd) {
  if (eventfd) {
    eventfd(0, 0); // Should not abort
  }
}
#endif

} // namespace
