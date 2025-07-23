#include "zos.h"
#include "gtest/gtest.h"
#include <fnmatch.h>

bool fnmatch_ok(const char* pattern, const char* str, int flags = 0) {
  return fnmatch(pattern, str, flags) == 0;
}

TEST(FnmatchTest, BasicPatterns) {
  EXPECT_TRUE(fnmatch_ok("*.txt", "file.txt"));
  EXPECT_FALSE(fnmatch_ok("*.txt", "file.TXT"));
  EXPECT_TRUE(fnmatch_ok("file*", "file123"));
  EXPECT_FALSE(fnmatch_ok("file*", "fil"));
}

TEST(FnmatchTest, CaseFoldFlag) {
  EXPECT_TRUE(fnmatch_ok("*.TXT", "file.txt", FNM_CASEFOLD));
  EXPECT_TRUE(fnmatch_ok("hello", "HELLO", FNM_CASEFOLD));
  EXPECT_TRUE(fnmatch_ok("HELLO", "hello", FNM_CASEFOLD));
  EXPECT_FALSE(fnmatch_ok("hello", "HELLO", 0));

  EXPECT_TRUE(fnmatch_ok("he*", "HELLO", FNM_CASEFOLD));
  EXPECT_TRUE(fnmatch_ok("HE*", "hello", FNM_CASEFOLD));
  EXPECT_FALSE(fnmatch_ok("he*", "HELLO", 0));

  EXPECT_TRUE(fnmatch_ok("h?llo", "HELLO", FNM_CASEFOLD));
  EXPECT_TRUE(fnmatch_ok("H?LLO", "hello", FNM_CASEFOLD));

  EXPECT_TRUE(fnmatch_ok("[a-z]ello", "HELLO", FNM_CASEFOLD));
  EXPECT_TRUE(fnmatch_ok("[A-Z]ELLO", "hello", FNM_CASEFOLD));
  EXPECT_FALSE(fnmatch_ok("[a-z]ello", "HELLO", 0));

  EXPECT_TRUE(fnmatch_ok("FoO*BaR", "foo123BAR", FNM_CASEFOLD));
  EXPECT_TRUE(fnmatch_ok("f*O?b*", "FOOBAR", FNM_CASEFOLD));
  EXPECT_FALSE(fnmatch_ok("FoO*BaR", "foo123BAR", 0));

  EXPECT_FALSE(fnmatch_ok("abc", "xyz", FNM_CASEFOLD));
  EXPECT_FALSE(fnmatch_ok("foo*", "barbaz", FNM_CASEFOLD));
}

TEST(FnmatchTest, NoEscapeFlag) {
  EXPECT_FALSE(fnmatch_ok("data\\*", "data*", FNM_NOESCAPE));
  EXPECT_TRUE(fnmatch_ok("data\\*", "data*", 0)); // \ is escape normally
}

TEST(FnmatchTest, PathnameFlag) {
  EXPECT_TRUE(fnmatch_ok("foo/bar", "foo/bar", FNM_PATHNAME));
  EXPECT_FALSE(fnmatch_ok("foo*", "foo/bar", FNM_PATHNAME)); // * doesn't match '/'
  EXPECT_TRUE(fnmatch_ok("*/bar", "foo/bar", FNM_PATHNAME));
}

TEST(FnmatchTest, EdgeCases) {
  EXPECT_TRUE(fnmatch_ok("", ""));
  EXPECT_TRUE(fnmatch_ok("*", ""));
  EXPECT_FALSE(fnmatch_ok("?", ""));
  EXPECT_TRUE(fnmatch_ok("f?o", "foo"));
  EXPECT_TRUE(fnmatch_ok("f[aeiou]o", "foo"));
  EXPECT_FALSE(fnmatch_ok("f[!aeiou]o", "foo"));
  EXPECT_TRUE(fnmatch_ok("f[!aeiou]o", "fzo"));
}
