// Enable CLIB overrides
#define ZOSLIB_OVERRIDE_CLIB 1

#include "zos.h"
#include "string.h"
#include "gtest/gtest.h"
#include <sys/file.h>
#include <sys/wait.h>

namespace {

TEST(FlockTest, BasicFileLocking) {
  char template_name[] = "/tmp/test_file_lock_XXXXXX";
  int fd = mkstemp(template_name);
  ASSERT_NE(fd, -1) << "Failed to create a unique temporary file";

  int ret = flock(fd, LOCK_EX);
  EXPECT_EQ(ret, 0) << "Failed to acquire exclusive lock";

  pid_t pid = fork();

  if (pid == 0) { // Child process, should fail to lock fd
    int childRet = flock(fd, LOCK_EX | LOCK_NB);
    EXPECT_EQ(childRet, -1) << "Child should fail to acquire exclusive lock";
      
    if (childRet == 0) {
      exit(EXIT_FAILURE);
    }
      
    exit(EXIT_SUCCESS);
  } else if (pid > 0) { // Parent process
      int status;
      pid_t childPid = wait(&status);
        
      EXPECT_EQ(WEXITSTATUS(status), EXIT_SUCCESS) << "Child process did not exit with success status";
      
      ret = flock(fd, LOCK_UN);
      EXPECT_EQ(ret, 0) << "Failed to release lock";

      close(fd);
      unlink(template_name);
  } else { // Fork failed
      FAIL() << "Failed to fork a child process";
  }
}

} // namespace
