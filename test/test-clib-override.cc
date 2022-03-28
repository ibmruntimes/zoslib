// Enable CLIB overrides
#define ZOSLIB_OVERRIDE_CLIB 1

#include "zos.h"
#include <fcntl.h>
#include <unistd.h>
#include "gtest/gtest.h"

namespace {

class CLIBOverrides : public ::testing::Test {
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

TEST_F(CLIBOverrides, open) {
    EXPECT_GE(fd, 0);
    // New files should be tagged as ASCII 819
    EXPECT_EQ(__getfdccsid(fd), 66355);

    // Should auto-convert to EBCDIC 1047
    fd = open("/etc/profile", O_RDONLY);
    EXPECT_EQ(__getfdccsid(fd), 66583);
    close(fd);

    // Should not auto-convert character files
    fd = open("/dev/tty", O_RDONLY);
    EXPECT_EQ(__getfdccsid(fd), 0);
    close(fd);
}

TEST_F(CLIBOverrides, pipe) {
    int pipefd[2];
    int rc = pipe(pipefd);
    EXPECT_GE(rc, 0);
    EXPECT_GE(pipefd[0], 0);
    EXPECT_GE(pipefd[1], 0);
    EXPECT_EQ(__getfdccsid(pipefd[0]), 66355);
    EXPECT_EQ(__getfdccsid(pipefd[1]), 66355);

    int ccsid = __getfdccsid(STDOUT_FILENO);
    dup2(STDOUT_FILENO, pipefd[0]);
    EXPECT_EQ(__getfdccsid(pipefd[0]), ccsid);
}
} // namespace
