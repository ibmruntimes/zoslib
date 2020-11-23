#include "zos.h"
#include "gtest/gtest.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

namespace {

static const char *ascii[] = {"A",
                              "AB",
                              "Hello, World!",
                              "0123456789",
                              "the quick brown fox jumps over the lazy dog",
                              "THE QUICK BROWN FOX JUMPS OVER THE LAZY DOG"};
#pragma convert("IBM-1047")
static const char *ebcdic[] = {"A",
                               "AB",
                               "Hello, World!",
                               "0123456789",
                               "the quick brown fox jumps over the lazy dog",
                               "THE QUICK BROWN FOX JUMPS OVER THE LAZY DOG"};
#pragma convert(pop)

TEST(E2ATest, ConvertE2A) {
  for (int i = 0; i < ARRAY_SIZE(ebcdic); i++) {
    const size_t len = strlen(ebcdic[i]) + 1;
    char *const buffer = new char[len];
    _convert_e2a(buffer, ebcdic[i], len);
    EXPECT_STREQ(ascii[i], buffer);
    delete[] buffer;
  }
}

TEST(E2ATest, E2AL) {
  for (int i = 0; i < ARRAY_SIZE(ebcdic); i++) {
    const size_t len = strlen(ebcdic[i]) + 1;
    char *const buffer = new char[len];
    strncpy(buffer, ebcdic[i], len);
    __e2a_l(buffer, len);
    EXPECT_STREQ(ascii[i], buffer);
    delete[] buffer;
  }
}

TEST(E2ATest, E2AS) {
  for (int i = 0; i < ARRAY_SIZE(ebcdic); i++) {
    const size_t len = strlen(ebcdic[i]) + 1;
    char *const buffer = new char[len];
    strncpy(buffer, ebcdic[i], len);
    __e2a_s(buffer);
    EXPECT_STREQ(ascii[i], buffer);
    delete[] buffer;
  }
}

TEST(A2ETest, ConvertA2E) {
  for (int i = 0; i < ARRAY_SIZE(ascii); i++) {
    const size_t len = strlen(ascii[i]) + 1;
    char *const buffer = new char[len];
    _convert_a2e(buffer, ascii[i], len);
    EXPECT_STREQ(ebcdic[i], buffer);
    delete[] buffer;
  }
}

TEST(A2ETest, A2EL) {
  for (int i = 0; i < ARRAY_SIZE(ascii); i++) {
    const size_t len = strlen(ascii[i]) + 1;
    char *const buffer = new char[len];
    strncpy(buffer, ascii[i], len);
    __a2e_l(buffer, len);
    EXPECT_STREQ(ebcdic[i], buffer);
    delete[] buffer;
  }
}

TEST(A2ETest, A2ES) {
  for (int i = 0; i < ARRAY_SIZE(ascii); i++) {
    const size_t len = strlen(ascii[i]) + 1;
    char *const buffer = new char[len];
    strncpy(buffer, ascii[i], len);
    __a2e_s(buffer);
    EXPECT_STREQ(ebcdic[i], buffer);
    delete[] buffer;
  }
}

} // namespace
