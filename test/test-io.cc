#include "zos.h"
#include <fcntl.h>
#include <sys/inotify.h>
#include <unistd.h>
#include "gtest/gtest.h"

namespace {

class ZOSIO : public ::testing::Test {
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

TEST_F(ZOSIO, ccsid) {
    EXPECT_GE(fd, 0);

    EXPECT_EQ(__setfdtext(fd), 0);
    EXPECT_EQ(__getfdccsid(fd), 0x10000 + 819);
}

} // namespace
