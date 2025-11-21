#include "zos.h"
#include "gtest/gtest.h"
#include <fts.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

// Test basic FTS open and close
TEST(FtsTest, OpenClose) {
  char *paths[] = {(char*)".", nullptr};
  
  FTS *fts = fts_open(paths, FTS_PHYSICAL, NULL);
  ASSERT_NE(fts, nullptr);
  
  int ret = fts_close(fts);
  EXPECT_EQ(ret, 0);
}

// Test FTS reading entries
TEST(FtsTest, ReadEntries) {
  char *paths[] = {(char*)".", nullptr};
  
  FTS *fts = fts_open(paths, FTS_PHYSICAL, NULL);
  ASSERT_NE(fts, nullptr);
  
  FTSENT *ent = fts_read(fts);
  ASSERT_NE(ent, nullptr);
  EXPECT_STREQ(ent->fts_name, ".");
  EXPECT_EQ(ent->fts_level, 0);
  
  fts_close(fts);
}

// Test FTS with logical walk
TEST(FtsTest, LogicalWalk) {
  char *paths[] = {(char*)".", nullptr};
  
  FTS *fts = fts_open(paths, FTS_LOGICAL, NULL);
  ASSERT_NE(fts, nullptr);
  
  FTSENT *ent = fts_read(fts);
  ASSERT_NE(ent, nullptr);
  
  fts_close(fts);
}

// Test FTS children function
TEST(FtsTest, Children) {
  char *paths[] = {(char*)".", nullptr};
  
  FTS *fts = fts_open(paths, FTS_PHYSICAL, NULL);
  ASSERT_NE(fts, nullptr);
  
  // Read root directory
  FTSENT *ent = fts_read(fts);
  ASSERT_NE(ent, nullptr);
  
  // Get children if it's a directory
  if (ent->fts_info == FTS_D) {
    FTSENT *child = fts_children(fts, 0);
    // Children may or may not exist, just verify the call works
  }
  
  fts_close(fts);
}

// Test FTS_NOCHDIR option
TEST(FtsTest, NoChdirOption) {
  char *paths[] = {(char*)".", nullptr};
  
  FTS *fts = fts_open(paths, FTS_PHYSICAL | FTS_NOCHDIR, NULL);
  ASSERT_NE(fts, nullptr);
  
  FTSENT *ent = fts_read(fts);
  ASSERT_NE(ent, nullptr);
  
  fts_close(fts);
}

// Test FTS error handling
TEST(FtsTest, InvalidPath) {
  char *paths[] = {(char*)"/nonexistent/path/that/should/not/exist", nullptr};
  
  FTS *fts = fts_open(paths, FTS_PHYSICAL, NULL);
  ASSERT_NE(fts, nullptr);
  
  FTSENT *ent = fts_read(fts);
  // Should get an error entry
  if (ent != nullptr) {
    if (ent->fts_info == FTS_NS || ent->fts_info == FTS_DNR || 
        ent->fts_info == FTS_ERR) {
      EXPECT_NE(ent->fts_errno, 0);
    }
  }
  
  fts_close(fts);
}

// Test FTS set function
TEST(FtsTest, SetInstruction) {
  char *paths[] = {(char*)".", nullptr};
  
  FTS *fts = fts_open(paths, FTS_PHYSICAL, NULL);
  ASSERT_NE(fts, nullptr);
  
  FTSENT *ent = fts_read(fts);
  ASSERT_NE(ent, nullptr);
  
  // Set instruction to skip
  int ret = fts_set(fts, ent, FTS_SKIP);
  EXPECT_EQ(ret, 0);
  
  fts_close(fts);
}

// Test FTS with comparison function
static int compare_entries(const FTSENT **a, const FTSENT **b) {
  return strcmp((*a)->fts_name, (*b)->fts_name);
}

TEST(FtsTest, WithComparator) {
  char *paths[] = {(char*)".", nullptr};
  
  FTS *fts = fts_open(paths, FTS_PHYSICAL, compare_entries);
  ASSERT_NE(fts, nullptr);
  
  FTSENT *ent = fts_read(fts);
  ASSERT_NE(ent, nullptr);
  
  fts_close(fts);
}
