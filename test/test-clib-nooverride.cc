#include "zos.h"

#include <fcntl.h>
#include <unistd.h>
#include "gtest/gtest.h"

namespace {

class CLIBNoOverrides : public ::testing::Test {
    virtual void SetUp() {
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

    // Should have no effect on reading 
    fd = open("/etc/profile", O_RDONLY);
    EXPECT_EQ(__getfdccsid(fd), 0);
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
} // namespace
