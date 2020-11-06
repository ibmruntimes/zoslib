#include "zos.h"

#include "gtest/gtest.h"

namespace {

static const char* ascii[] = { "A", "AB", "Hello, World!", "0123456789",
                               "the quick brown fox jumps over the lazy dog",
                               "THE QUICK BROWN FOX JUMPS OVER THE LAZY DOG" };
#pragma convert("IBM-1047")
static const char* ebcdic[] = { "A", "AB", "Hello, World!", "0123456789",
                                "the quick brown fox jumps over the lazy dog",
                                "THE QUICK BROWN FOX JUMPS OVER THE LAZY DOG" };
#pragma convert(pop)

TEST(E2ATest, ConvertE2A) {
  for (int i = 0; i < sizeof(ebcdic) / sizeof(ebcdic[0]); i++) {
    const size_t len = strlen(ascii[i]) + 1;
    char* const buffer = new char[len];
    _convert_e2a(buffer, ebcdic[i], len);
    EXPECT_STREQ(ascii[i], buffer);
    delete[] buffer;
  }
}

TEST(A2ETest, ConvertA2E) {
  for (int i = 0; i < sizeof(ascii) / sizeof(ascii[0]); i++) {
    const size_t len = strlen(ebcdic[i]) + 1;
    char* const buffer = new char[len];
    _convert_a2e(buffer, ascii[i], len);
    EXPECT_STREQ(ebcdic[i], buffer);
    delete[] buffer;
  }
}

}
