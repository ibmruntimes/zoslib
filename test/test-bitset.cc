#include "zos.h"
#include "gtest/gtest.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

namespace {

static const unsigned long nums[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
                                     0xFFFFFFFFFFFFFF,
                                     0xF0F0F0F0F0F0F0};

TEST(BitSetTest, BSAddOne) {
  for (int i = 0; i < ARRAY_SIZE(nums); i++) {
    std::bitset<64> bs(nums[i]); 
    std::bitset<64> bsr = ::__addOne(bs);
    EXPECT_EQ(bsr.to_ulong(), bs.to_ulong() + 1);
  }
}

TEST(BitSetTest, BSSubtractOne) {
  for (int i = 0; i < ARRAY_SIZE(nums); i++) {
    std::bitset<64> bs(nums[i]); 
    std::bitset<64> bsr = ::__subtractOne(bs);
    EXPECT_EQ(bsr.to_ulong(), bs.to_ulong() - 1);
  }
}

} // namespace
