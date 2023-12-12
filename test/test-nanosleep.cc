#include "test-args.h"
#include "zos.h"
#include "gtest/gtest.h"

namespace {

TEST(ArgsTest, NanoSleepCC) {
  int rc;
  struct timespec timeout = { 0, 1000000u }; // 1ms
  for (int i=0; i<2000; i++) {
    rc = nanosleep(&timeout, NULL);
    if (rc != 0)
      perror("nanosleep");
    EXPECT_EQ(rc, 0);
  }
}

}
