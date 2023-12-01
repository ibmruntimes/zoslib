#include "zos.h"
#include "gtest/gtest.h"
#include <libgen.h>

namespace {

TEST(ProgramInfoTest, GetProgDir) {
  const char* path = __getprogramdir();
  EXPECT_STRNE(path, NULL);

  // Get program path in a different way
  char** argv = __getargv();

  // Check if program path is equal to the current working directory
  EXPECT_STREQ(path, dirname(__realpath_extended(argv[0], NULL)));
}

TEST(ProgramInfoTest, GetProgName) {
  const char* path = getprogname();
  const char* substring = "cctest";

  // Get program name in a different way
  char** argv = __getargv();

  EXPECT_STREQ(path, basename(argv[0]));
}

} // namespace
