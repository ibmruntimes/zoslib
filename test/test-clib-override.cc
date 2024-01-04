// Enable CLIB overrides
#define ZOSLIB_OVERRIDE_CLIB 1

#include "zos.h"
#include <fcntl.h>
#include <sys/inotify.h>
#include <unistd.h>
#include "gtest/gtest.h"

namespace {

class CLIBOverrides : public ::testing::Test {
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

TEST_F(CLIBOverrides, open) {
    EXPECT_GE(fd, 0);
    // New files should be tagged as ASCII 819
    EXPECT_EQ(__getfdccsid(fd), 0x10000 + 819);
    close(fd);

    // Should auto-convert untagged files to EBCDIC 1047
    fd = open("/etc/profile", O_RDONLY);
    if (fd != -1) {
      EXPECT_EQ(__getfdccsid(fd), 0x10000 + 1047);
      close(fd);
    }

    // Should not auto-convert character files
    fd = open("/dev/random", O_RDONLY);
    if (fd != -1) {
      EXPECT_EQ(__getfdccsid(fd), 0);
      close(fd);
    }

    // Delete and re-open temp_path with only read permissions
    remove(temp_path);
    fd = open(temp_path, O_CREAT | O_RDONLY, S_IRUSR);
    EXPECT_EQ(__getfdccsid(fd), 0x10000 + 819);
    close(fd);

    char buff[] = "This is a test";
    char* buff2 = (char*)malloc(sizeof(buff));

#if CHECK_NFS
    const char* file_str = "/gsa/tlbgsa/projects/i/igortest/nodejs_data_file";
    fd = open(file_str, O_CREAT | O_WRONLY, 0777);
    write(fd, buff, sizeof(buff));
    close(fd);

    fd = open(file_str, O_RDONLY);
    memset(buff2, sizeof(buff), 1);
    read(fd, buff2, sizeof(buff));
    EXPECT_EQ(strcmp(buff, buff2), 0);
    close(fd);
#endif

    // Delete and re-open temp_path _ENCODE_FILE_NEW=IBM-1047

    setenv("_ENCODE_FILE_NEW", "IBM-1047", 1);
    remove(temp_path);
    fd = open(temp_path, O_CREAT | O_WRONLY, 0777);
    EXPECT_EQ(__getfdccsid(fd), 0x10000 + 1047);
    write(fd, buff, sizeof(buff));
    close(fd);

    fd = open(temp_path, O_RDONLY);
    EXPECT_EQ(__getfdccsid(fd), 0x10000 + 1047);
    memset(buff2, 1, sizeof(buff));
    read(fd, buff2, sizeof(buff));
    EXPECT_EQ(strcmp(buff, buff2), 0);
    close(fd);

    // Delete and re-open temp_path _ENCODE_FILE_NEW=BINARY
    setenv("_ENCODE_FILE_NEW", "BINARY", 1);
    remove(temp_path);
    fd = open(temp_path, O_CREAT | O_WRONLY, 0777);
    EXPECT_EQ(__getfdccsid(fd), 65535);
    write(fd, buff, sizeof(buff));
    close(fd);

    fd = open(temp_path, O_RDONLY);
    EXPECT_EQ(__getfdccsid(fd), 65535);
    memset(buff2, 1, sizeof(buff));
    read(fd, buff2, sizeof(buff));
    EXPECT_EQ(strcmp(buff, buff2), 0);
    close(fd);

    // Delete and re-open temp_path _ENCODE_FILE_NEW=ISO8859-1
    setenv("_ENCODE_FILE_NEW", "ISO8859-1", 1);
    remove(temp_path);
    fd = open(temp_path, O_CREAT | O_WRONLY, 0777);
    EXPECT_EQ(__getfdccsid(fd), 0x10000 + 819);
    write(fd, buff, sizeof(buff));
    close(fd);

    fd = open(temp_path, O_RDONLY);
    EXPECT_EQ(__getfdccsid(fd), 0x10000 + 819);
    memset(buff2, 1, sizeof(buff));
    read(fd, buff2, sizeof(buff));
    EXPECT_EQ(strcmp(buff, buff2), 0);
    free(buff2);
    close(fd);
    unsetenv("_ENCODE_FILE_NEW");
}

TEST_F(CLIBOverrides, pipe) {
    int pipefd[2];
    int rc = pipe(pipefd);
    EXPECT_GE(rc, 0);
    EXPECT_GE(pipefd[0], 0);
    EXPECT_GE(pipefd[1], 0);
    EXPECT_NE(pipefd[0], pipefd[1]);
    EXPECT_EQ(__getfdccsid(pipefd[0]), 0x10000 + 819);
    EXPECT_EQ(__getfdccsid(pipefd[1]), 0x10000 + 819);

    int ccsid = __getfdccsid(STDOUT_FILENO);
    dup2(STDOUT_FILENO, pipefd[0]);
    EXPECT_EQ(__getfdccsid(pipefd[0]), ccsid);
}


TEST_F(CLIBOverrides, socketpair) {
    int fd[2];
    int rc = socketpair(AF_UNIX, SOCK_STREAM, 0, fd);
    EXPECT_GE(rc, 0);
    EXPECT_GE(fd[0], 0);
    EXPECT_GE(fd[1], 0);
    EXPECT_NE(fd[0], fd[1]);
#if defined(ZOSLIB_ENABLE_V2R5_FEATURES)
    if (__is_os_level_at_or_above(ZOSLVL_V2R5) && inotify_init) {
      // Auto-convert is enabled to the program CCSID (819)
      EXPECT_EQ(__getfdccsid(fd[0]), 0x10000);
      EXPECT_EQ(__getfdccsid(fd[1]), 0x10000);
    } else {
#endif
      EXPECT_EQ(__getfdccsid(fd[0]), 0);
      EXPECT_EQ(__getfdccsid(fd[1]), 0);
#if defined(ZOSLIB_ENABLE_V2R5_FEATURES)
    }
#endif
}
} // namespace
