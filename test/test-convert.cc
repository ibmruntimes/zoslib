#include "zos.h"

#include "gtest/gtest.h"

namespace {

TEST(E2ATest, ConvertE2A) {
  auto test_convert_e2a = [](const char* ebcdic, const char* ascii) {
    const size_t len = strlen(ascii) + 1;
    char* const buffer = new char[len];
    _convert_e2a(buffer, ebcdic, len);
    EXPECT_STREQ(ascii, buffer);
    delete[] buffer;
  };

  test_convert_e2a("\xC1", "A");
  test_convert_e2a("\xC1\xC2", "AB");
  test_convert_e2a("\xC8\x85\x93\x93\x96\x6B\x40\xE6\x96\x99\x93\x84\x5A",
                   "Hello, World!");
  test_convert_e2a("\xF0\xF1\xF2\xF3\xF4\xF5\xF6\xF7\xF8\xF9", "0123456789");
  test_convert_e2a("\xA3\x88\x85\x40\x98\xA4\x89\x83\x92\x40\x82\x99\x96\xA6"
                   "\x95\x40\x86\x96\xA7\x40\x91\xA4\x94\x97\xA2\x40\x96\xA5"
                   "\x85\x99\x40\xA3\x88\x85\x40\x93\x81\xA9\xA8\x40\x84\x96"
                   "\x87",
                   "the quick brown fox jumps over the lazy dog");
  test_convert_e2a("\xE3\xC8\xC5\x40\xD8\xE4\xC9\xC3\xD2\x40\xC2\xD9\xD6\xE6"
                   "\xD5\x40\xC6\xD6\xE7\x40\xD1\xE4\xD4\xD7\xE2\x40\xD6\xE5"
                   "\xC5\xD9\x40\xE3\xC8\xC5\x40\xD3\xC1\xE9\xE8\x40\xC4\xD6"
                   "\xC7",
                   "THE QUICK BROWN FOX JUMPS OVER THE LAZY DOG");
}

TEST(A2ETest, ConvertA2E) {
  auto test_convert_a2e = [](const char* ascii, const char* ebcdic) {
    const size_t len = strlen(ebcdic) + 1;
    char* const buffer = new char[len];
    _convert_a2e(buffer, ascii, len);
    EXPECT_STREQ(ebcdic, buffer);
    delete[] buffer;
  };

  test_convert_a2e("A", "\xC1");
  test_convert_a2e("AB", "\xC1\xC2");
  test_convert_a2e("Hello, World!",
                   "\xC8\x85\x93\x93\x96\x6B\x40\xE6\x96\x99\x93\x84\x5A");
  test_convert_a2e("0123456789", "\xF0\xF1\xF2\xF3\xF4\xF5\xF6\xF7\xF8\xF9");
  test_convert_a2e("the quick brown fox jumps over the lazy dog",
                   "\xA3\x88\x85\x40\x98\xA4\x89\x83\x92\x40\x82\x99\x96\xA6"
                   "\x95\x40\x86\x96\xA7\x40\x91\xA4\x94\x97\xA2\x40\x96\xA5"
                   "\x85\x99\x40\xA3\x88\x85\x40\x93\x81\xA9\xA8\x40\x84\x96"
                   "\x87");
  test_convert_a2e("THE QUICK BROWN FOX JUMPS OVER THE LAZY DOG",
                   "\xE3\xC8\xC5\x40\xD8\xE4\xC9\xC3\xD2\x40\xC2\xD9\xD6\xE6"
                   "\xD5\x40\xC6\xD6\xE7\x40\xD1\xE4\xD4\xD7\xE2\x40\xD6\xE5"
                   "\xC5\xD9\x40\xE3\xC8\xC5\x40\xD3\xC1\xE9\xE8\x40\xC4\xD6"
                   "\xC7");
}

}
