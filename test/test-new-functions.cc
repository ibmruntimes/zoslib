#include "zos.h"
#include "gtest/gtest.h"
#include <libgen.h>

namespace {

TEST(NewFunctionsTest, GetProgDir) {
  const char* path = __getprogramdir();
  EXPECT_STRNE(path, NULL);

  // get program path in a different way
  char** argv = __getargv();

  // Check if program path is equal to the current working directory
  EXPECT_STREQ(path, dirname(__realpath_extended(argv[0], NULL)));
}

TEST(NewFunctionsTest, GetProgName) {
  const char* path = getprogname();
  const char* substring = "cctest";

  // get program name in a different way
  char** argv = __getargv();

  EXPECT_STREQ(path, basename(argv[0]));
}

} // namespace
