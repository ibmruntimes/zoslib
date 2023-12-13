#include "test-args.h"
#include "zos.h"
#include "gtest/gtest.h"
#include <sys/stat.h>
#include <unistd.h>

namespace {

TEST(MkdtempTest, MkdtempCC) {
  char templ[] = "/tmp/dirXXXXXX";
  char * dirname = mkdtemp(templ);
  EXPECT_NE(dirname, nullptr);

  struct stat stats;
  int rc = stat(dirname, &stats);
  rmdir(dirname);
  EXPECT_EQ(rc, 0);
  EXPECT_TRUE(S_ISDIR(stats.st_mode));
}

}
