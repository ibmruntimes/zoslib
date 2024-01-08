// Enable CLIB overrides
#define ZOSLIB_OVERRIDE_CLIB 1

#include "zos.h"
#include "string.h"
#include "gtest/gtest.h"
#include <sys/file.h>

namespace {

TEST(FlockTest, BasicFileLocking) {
  char template_name[] = "/tmp/test_file_lock_XXXXXX";
  int fd = mkstemp(template_name);

  ASSERT_NE(fd, 0) << "Failed to create a unique temporary file";
  int fd = open(filename, O_RDWR | O_CREAT, 0666);
  ASSERT_NE(fd, -1) << "Failed to open file for testing";

  int ret = flock(fd, LOCK_EX);
  EXPECT_EQ(ret, 0) << "Failed to acquire exclusive lock";

  int sharedRet = flock(fd, LOCK_SH | LOCK_NB);
  EXPECT_NE(sharedRet, 0) << "Should not be able to acquire shared lock when exclusive is held";

  ret = flock(fd, LOCK_UN);
  EXPECT_EQ(ret, 0) << "Failed to release lock";

  close(fd);

  unlink(filename);
}

TEST(FlockTest, NonBlockingLock) {
  char template_name[] = "/tmp/test_file_lock_nonblocking_XXXXXX";
  
  int fd = mkstemp(template_name);
  ASSERT_NE(fd, -1) << "Failed to create a unique temporary file";

  int ret = flock(fd, LOCK_EX | LOCK_NB);
  EXPECT_EQ(ret, 0) << "Failed to acquire non-blocking exclusive lock";

  int ret2 = flock(fd, LOCK_EX | LOCK_NB);
  EXPECT_NE(ret2, 0) << "Should not be able to acquire non-blocking exclusive lock again";

  ret = flock(fd, LOCK_UN);
  EXPECT_EQ(ret, 0) << "Failed to release lock";

  close(fd);

  unlink(template_name);
}

} // namespace
