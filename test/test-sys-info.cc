#include "zos.h"
#include "gtest/gtest.h"

namespace {

TEST(SysInfoTest, NumOnlineCPUs) {
  EXPECT_GE(__get_num_online_cpus(), 0);
}

TEST(SysInfoTest, NumFrames) {
  EXPECT_GE(__get_num_frames(), 0);
}

TEST(SysInfoTest, LoadAvg) {
  double load[3];
  EXPECT_EQ(getloadavg(load, 1), 1);
  EXPECT_EQ(getloadavg(load, 3), 3);
  EXPECT_EQ(getloadavg(load, 4), -1);
  EXPECT_GE(load[0], 0.0);
  EXPECT_GE(load[1], 0.0);
  EXPECT_GE(load[2], 0.0);
}

TEST(SysInfoTest, CPUModel) {
  size_t size = ZOSCPU_MODEL_LENGTH + 1;
  char model[size];
  __get_cpu_model(model, size);
  int ccsid = __guess_ue(model, size, NULL, 0);
  EXPECT_EQ(ccsid, 819);
}

} // namespace
