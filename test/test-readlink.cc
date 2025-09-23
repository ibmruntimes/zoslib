#include <gtest/gtest.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

// Forward declaration for the wrapper implementation
extern "C" ssize_t __readlink(const char *path, char *buf, size_t bufsiz);

class ReadlinkTest : public ::testing::Test {
protected:
  void SetUp() override {
    test_dir = "/tmp/readlink_test_dir";
    mkdir(test_dir, 0755);

    target_file = std::string(test_dir) + "/$target";
    symlink_with_dollar = std::string(test_dir) + "/$dollar";
    normal_symlink = std::string(test_dir) + "/normal_symlink";
    unresolved_symlink = std::string(test_dir) + "/unresolved_symlink";

    int fd = open(target_file.c_str(), O_CREAT | O_WRONLY, 0644);
    ASSERT_NE(fd, -1);
    write(fd, "test content", 12);
    close(fd);

    ASSERT_EQ(symlink(target_file.c_str(), symlink_with_dollar.c_str()), 0);
    ASSERT_EQ(symlink(target_file.c_str(), normal_symlink.c_str()), 0);
#if 0 // TODO: add a test for sysplex
    ASSERT_EQ(symlink("$VERSION/", unresolved_symlink.c_str()), -1);
#endif
  }

  void TearDown() override {
    unlink(symlink_with_dollar.c_str());
    unlink(normal_symlink.c_str());
    unlink(target_file.c_str());
    unresolved_symlink = std::string(test_dir) + "/unresolved_symlink";
    rmdir(test_dir);
  }

  const char *test_dir;
  std::string target_file;
  std::string symlink_with_dollar;
  std::string normal_symlink;
  std::string unresolved_symlink;
};

TEST_F(ReadlinkTest, SymlinkWithDollarInName) {
  // Test a symlink with "$" in its name
  char buf[PATH_MAX];
  ssize_t len = __readlink(symlink_with_dollar.c_str(), buf, sizeof(buf));
  ASSERT_GT(len, 0);
  buf[len] = '\0'; 
  EXPECT_STREQ(buf, target_file.c_str());
}

TEST_F(ReadlinkTest, NormalSymlink) {
  // Test a normal symlink (no "$" in its name)
  char buf[PATH_MAX];
  ssize_t len = __readlink(normal_symlink.c_str(), buf, sizeof(buf));
  ASSERT_GT(len, 0);
  buf[len] = '\0'; 
  EXPECT_STREQ(buf, target_file.c_str());
}

#if 0
TEST_F(ReadlinkTest, UnresolvedSymlinkWithDollarTarget) {
  // Test a symlink where the target starts with "$VERSION/"
  char buf[PATH_MAX];
  ssize_t len = __readlink(unresolved_symlink.c_str(), buf, sizeof(buf));
  ASSERT_GT(len, 0);
  buf[len] = '\0'; 

  EXPECT_STREQ(buf, "/");
}
#endif
