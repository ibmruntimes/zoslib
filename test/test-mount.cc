#include <sys/mount.h>
#include <stdio.h>
#include "zos.h"
#include "gtest/gtest.h"

namespace {

TEST(MountTest, GetMntInfo) {
  struct statfs* mntbufp;
  int entries = getmntinfo(&mntbufp, MNT_NOWAIT);
  EXPECT_GE(entries, 0);
  int i;
  if (entries > 0) {
    for (i=0; i<entries; ++i) {
      size_t fromlen = strlen(mntbufp[i].f_mntfromname);
      size_t onlen = strlen(mntbufp[i].f_mntonname);
      size_t typelen = strlen(mntbufp[i].f_fstypename);
      EXPECT_GE(fromlen, 1);
      EXPECT_GE(typelen, 1);
    }
  } 
}
} // namespace

