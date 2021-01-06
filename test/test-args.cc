#include "zos.h"
#include "gtest/gtest.h"
#include "test-args.h"

namespace {

TEST(ArgsTest, GetArgc) {
  int argc = __getargc();
  EXPECT_EQ(expected_argc, argc);
}

TEST(ArgsTest, GetArgv) {
  char **argv = __getargv();
  for (int i = 0; i < expected_argc; i++) {
    EXPECT_STREQ(expected_argv[i], argv[i]);
  }
}

} // namespace
