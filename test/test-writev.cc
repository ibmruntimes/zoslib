#define ZOSLIB_OVERRIDE_CLIB 1

#include "zos.h"
#include <gtest/gtest.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/uio.h>
#include <string.h>
#include <pty.h>

static const char *ascii[] = {"A",
                              "AB",
                              "Hello, World!",
                              "0123456789",
                              "the quick brown fox jumps over the lazy dog",
                              "THE QUICK BROWN FOX JUMPS OVER THE LAZY DOG"};
#pragma convert("IBM-1047")
static const char *ebcdic[] = {"A",
                               "AB",
                               "Hello, World!",
                               "0123456789",
                               "the quick brown fox jumps over the lazy dog",
                               "THE QUICK BROWN FOX JUMPS OVER THE LAZY DOG"};
#pragma convert(pop)

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

class WritevTest : public ::testing::Test {
protected:
    void tagFile(int fd, int ccsid) {
        __setfdccsid(fd, ccsid);
    }

    void SetUp() override {
        ASSERT_EQ(openpty(&master_fd, &slave_fd, NULL, NULL, NULL), 0) << "Failed to open pseudo-terminal";
    }

    void TearDown() override {
        close(master_fd);
        close(slave_fd);
    }
protected:
    int master_fd, slave_fd;
};

// This tests the writev function when ASCII data is passed.
TEST_F(WritevTest, AsciiDataWrite) {
    struct iovec iov[6];
    size_t total_len = 0;

    for (int i = 0; i < sizeof(ascii) / sizeof(ascii[0]); ++i) {
        iov[i].iov_base = (void*)ascii[i];
        iov[i].iov_len = strlen(ascii[i]);
        total_len += iov[i].iov_len;
    }

    ssize_t result = writev(slave_fd, iov, 6);
    ASSERT_EQ(result, (ssize_t)total_len);

    char buffer[1024] = {0};
    ssize_t read_bytes = read(master_fd, buffer, sizeof(buffer) - 1);
    ASSERT_GT(read_bytes, 0);

    size_t offset = 0;
    for (size_t i = 0; i < ARRAY_SIZE(ascii); i++) {
        size_t len = strlen(ascii[i]);
        ASSERT_EQ(memcmp(buffer + offset, ascii[i], len), 0);
        offset += len;
    }
}

// This tests the writev function when EBCDIC data is passed.
TEST_F(WritevTest, EbcdicDataWrite) {
    struct iovec iov[6];
    size_t total_len = 0;

    for (int i = 0; i < sizeof(ebcdic) / sizeof(ebcdic[0]); ++i) {
        iov[i].iov_base = (void*)ebcdic[i];
        iov[i].iov_len = strlen(ebcdic[i]);
        total_len += iov[i].iov_len;
    }

    ssize_t result = writev(slave_fd, iov, 6);
    ASSERT_EQ(result, (ssize_t)total_len);

    char buffer[1024] = {0};
    ssize_t read_bytes = read(master_fd, buffer, sizeof(buffer) - 1);
    ASSERT_GT(read_bytes, 0);

    size_t offset = 0;
    for (size_t i = 0; i < ARRAY_SIZE(ebcdic); i++) {
        size_t len = strlen(ebcdic[i]);
        ASSERT_EQ(memcmp(buffer + offset, ebcdic[i], len), 0);
        offset += len;
    }
}
