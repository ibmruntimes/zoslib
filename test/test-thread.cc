// Enable CLIB overrides
#define ZOSLIB_OVERRIDE_CLIB 1

#include "zos.h"
#include "gtest/gtest.h"
#include <sys/file.h>
#include <sys/wait.h>
#include <stdio.h>
#include <pthread.h>

int cvsstate = 0;

namespace {

void* print_cvstate(void* arg) {
  int* cvsstate_ptr = static_cast<int*>(arg);
  *cvsstate_ptr = __ae_autoconvert_state(_CVTSTATE_QUERY);

  return NULL;
}

TEST(CvstateTest, PrintCvstate) {
  {
  int cvsstate = 0;
  unsetenv("_BPXK_AUTOCVT");

  pthread_t thread;
  pthread_create(&thread, NULL, &print_cvstate, &cvsstate);

  pthread_join(thread, NULL);

  EXPECT_EQ(cvsstate, _CVTSTATE_ON);
  }

  {
  int cvsstate = 0;
  setenv("_BPXK_AUTOCVT", "ON", 1);

  pthread_t thread;
  pthread_create(&thread, NULL, &print_cvstate, &cvsstate);

  pthread_join(thread, NULL);

  EXPECT_EQ(cvsstate, _CVTSTATE_ON);
  }
}

}

