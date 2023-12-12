#undef ZOSLIB_OVERRIDE_CLIB

#include "zos.h"

#include <fcntl.h>
#include <unistd.h>
#include "gtest/gtest.h"

namespace {

class CLIBNoOverrides : public ::testing::Test {
    virtual void SetUp() {
      // Make sure default untagged read mode is set
      setenv("UNTAGGED_READ_MODE", "AUTO", 1);
      temp_path = tmpnam(NULL);
      fd = open(temp_path, O_CREAT | O_WRONLY, 0660);
    }

    virtual void TearDown() {
      close(fd);
      remove(temp_path);
    }

protected:
    char* temp_path;
    int fd;
};

TEST_F(CLIBNoOverrides, open) {
    EXPECT_GE(fd, 0);
    // New files should be untagged
    EXPECT_EQ(__getfdccsid(fd), 0);
    close(fd);

    // Should have no auto conversion on reading
    fd = open("/etc/profile", O_RDONLY);
    if (fd >= 0) {
      EXPECT_EQ(__getfdccsid(fd), 0);
      close(fd);
    }

    // Should not auto-convert character files
    fd = open("/dev/random", O_RDONLY);
    if (fd >= 0) {
      EXPECT_EQ(__getfdccsid(fd), 0);
      close(fd);
    }

    // Delete and re-open temp_path with only read permissions
    remove(temp_path);
    fd = open(temp_path, O_CREAT | O_RDONLY, S_IRUSR);
    EXPECT_EQ(__getfdccsid(fd), 0);
    close(fd);
}

TEST_F(CLIBNoOverrides, pipe) {
    int pipefd[2];
    int rc = pipe(pipefd);
    EXPECT_GE(rc, 0);
    EXPECT_GE(pipefd[0], 0);
    EXPECT_GE(pipefd[1], 0);
    EXPECT_EQ(__getfdccsid(pipefd[0]), 0);
    EXPECT_EQ(__getfdccsid(pipefd[1]), 0);

    int ccsid = __getfdccsid(STDOUT_FILENO);
    dup2(STDOUT_FILENO, pipefd[0]);
    EXPECT_EQ(__getfdccsid(pipefd[0]), ccsid);
}

TEST_F(CLIBNoOverrides, socketpair) {
    int fd[2];
    int rc = socketpair(AF_UNIX, SOCK_STREAM, 0, fd);
    EXPECT_GE(rc, 0);
    EXPECT_GE(fd[0], 0);
    EXPECT_GE(fd[1], 0);
    EXPECT_NE(fd[0], fd[1]);
    EXPECT_EQ(__getfdccsid(fd[0]), 0);
    EXPECT_EQ(__getfdccsid(fd[1]), 0);
}
} // namespace
