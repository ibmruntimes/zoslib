#include <sys/mount.h>
#include <stdio.h>

int main() {
  struct statfs* mntbufp;
  int entries = getmntinfo(&mntbufp, MNT_NOWAIT);
  int i;
  if (entries > 0) {
    printf("%d entries\n", entries);
    for (i=0; i<entries; ++i) {
      printf("%s mounted at %s, file type %s\n", mntbufp[i].f_mntfromname, mntbufp[i].f_mntonname, mntbufp[i].f_fstypename);
    }
  } else {
    printf("rc: %d from getmntinfo\n", entries);
  }
  return 0;
}
