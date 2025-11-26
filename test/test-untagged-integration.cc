///////////////////////////////////////////////////////////////////////////////
// Licensed Materials - Property of IBM
// ZOSLIB
// (C) Copyright IBM Corp. 2025. All Rights Reserved.
// US Government Users Restricted Rights - Use, duplication
// or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
///////////////////////////////////////////////////////////////////////////////

#include "gtest/gtest.h"
#include "zos-base.h"
#include <stdlib.h>

/**
 * Integration tests to verify __UNTAGGED_FILE_ENCODING is properly
 * integrated with existing zoslib behavior.
 */

class UntaggedIntegrationTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Clear environment variables before each test
    unsetenv("__UNTAGGED_FILE_ENCODING");
    unsetenv("__UNTAGGED_READ_MODE");
    // Force re-initialization of settings
    __update_envar_settings(NULL);
  }

  void TearDown() override {
    // Clean up after each test
    unsetenv("__UNTAGGED_FILE_ENCODING");
    unsetenv("__UNTAGGED_READ_MODE");
    __update_envar_settings(NULL);
  }
};

// Test that new variable integrates with __get_no_tag_read_behaviour
TEST_F(UntaggedIntegrationTest, NewVariableSetsLegacyBehavior) {
  // DETECT should map to __NO_TAG_READ_DEFAULT
  setenv("__UNTAGGED_FILE_ENCODING", "DETECT", 1);
  __update_envar_settings("__UNTAGGED_READ_MODE");
  EXPECT_EQ(__NO_TAG_READ_DEFAULT, __get_no_tag_read_behaviour());

  // WARN should map to __NO_TAG_READ_DEFAULT_WITHWARNING
  setenv("__UNTAGGED_FILE_ENCODING", "WARN", 1);
  __update_envar_settings("__UNTAGGED_READ_MODE");
  EXPECT_EQ(__NO_TAG_READ_DEFAULT_WITHWARNING, __get_no_tag_read_behaviour());

  // IGNORE should map to __NO_TAG_READ_STRICT
  setenv("__UNTAGGED_FILE_ENCODING", "IGNORE", 1);
  __update_envar_settings("__UNTAGGED_READ_MODE");
  EXPECT_EQ(__NO_TAG_READ_STRICT, __get_no_tag_read_behaviour());

  // Explicit CCSID should map to __NO_TAG_READ_V6
  setenv("__UNTAGGED_FILE_ENCODING", "1047", 1);
  __update_envar_settings("__UNTAGGED_READ_MODE");
  EXPECT_EQ(__NO_TAG_READ_V6, __get_no_tag_read_behaviour());
}

// Test that CCSID is stored correctly
TEST_F(UntaggedIntegrationTest, CCSIDIsStored) {
  setenv("__UNTAGGED_FILE_ENCODING", "1047", 1);
  __update_envar_settings("__UNTAGGED_READ_MODE");
  EXPECT_EQ(1047, __get_untagged_file_ccsid());

  setenv("__UNTAGGED_FILE_ENCODING", "UTF-8", 1);
  __update_envar_settings("__UNTAGGED_READ_MODE");
  EXPECT_EQ(1208, __get_untagged_file_ccsid());

  setenv("__UNTAGGED_FILE_ENCODING", "819", 1);
  __update_envar_settings("__UNTAGGED_READ_MODE");
  EXPECT_EQ(819, __get_untagged_file_ccsid());

  // Semantic tokens should have CCSID 0
  setenv("__UNTAGGED_FILE_ENCODING", "DETECT", 1);
  __update_envar_settings("__UNTAGGED_READ_MODE");
  EXPECT_EQ(0, __get_untagged_file_ccsid());
}

// Test precedence: new variable overrides legacy
TEST_F(UntaggedIntegrationTest, NewVariableOverridesLegacy) {
  setenv("__UNTAGGED_READ_MODE", "STRICT", 1);
  setenv("__UNTAGGED_FILE_ENCODING", "1047", 1);
  __update_envar_settings("__UNTAGGED_READ_MODE");

  // New variable should take precedence, resulting in V6 mode (forced conversion)
  EXPECT_EQ(__NO_TAG_READ_V6, __get_no_tag_read_behaviour());
  EXPECT_EQ(1047, __get_untagged_file_ccsid());
}

// Test legacy variable still works when new variable not set
TEST_F(UntaggedIntegrationTest, LegacyVariableStillWorks) {
  setenv("__UNTAGGED_READ_MODE", "WARN", 1);
  __update_envar_settings("__UNTAGGED_READ_MODE");
  EXPECT_EQ(__NO_TAG_READ_DEFAULT_WITHWARNING, __get_no_tag_read_behaviour());

  setenv("__UNTAGGED_READ_MODE", "STRICT", 1);
  __update_envar_settings("__UNTAGGED_READ_MODE");
  EXPECT_EQ(__NO_TAG_READ_STRICT, __get_no_tag_read_behaviour());

  setenv("__UNTAGGED_READ_MODE", "ASCII", 1);
  __update_envar_settings("__UNTAGGED_READ_MODE");
  EXPECT_EQ(__NO_TAG_READ_V6, __get_no_tag_read_behaviour());
  EXPECT_EQ(819, __get_untagged_file_ccsid());
}

// Test encoding name resolution
TEST_F(UntaggedIntegrationTest, EncodingNameResolution) {
  setenv("__UNTAGGED_FILE_ENCODING", "IBM-1047", 1);
  __update_envar_settings("__UNTAGGED_READ_MODE");
  EXPECT_EQ(__NO_TAG_READ_V6, __get_no_tag_read_behaviour());
  EXPECT_EQ(1047, __get_untagged_file_ccsid());

  setenv("__UNTAGGED_FILE_ENCODING", "ASCII", 1);
  __update_envar_settings("__UNTAGGED_READ_MODE");
  EXPECT_EQ(__NO_TAG_READ_V6, __get_no_tag_read_behaviour());
  EXPECT_EQ(819, __get_untagged_file_ccsid());
}

// Test case insensitivity
TEST_F(UntaggedIntegrationTest, CaseInsensitivity) {
  setenv("__UNTAGGED_FILE_ENCODING", "detect", 1);
  __update_envar_settings("__UNTAGGED_READ_MODE");
  EXPECT_EQ(__NO_TAG_READ_DEFAULT, __get_no_tag_read_behaviour());

  setenv("__UNTAGGED_FILE_ENCODING", "utf-8", 1);
  __update_envar_settings("__UNTAGGED_READ_MODE");
  EXPECT_EQ(__NO_TAG_READ_V6, __get_no_tag_read_behaviour());
  EXPECT_EQ(1208, __get_untagged_file_ccsid());
}

// Test invalid values fallback to default
TEST_F(UntaggedIntegrationTest, InvalidValuesFallback) {
  setenv("__UNTAGGED_FILE_ENCODING", "INVALID", 1);
  __update_envar_settings("__UNTAGGED_READ_MODE");
  EXPECT_EQ(__NO_TAG_READ_DEFAULT, __get_no_tag_read_behaviour());
  EXPECT_EQ(0, __get_untagged_file_ccsid());

  unsetenv("__UNTAGGED_FILE_ENCODING");
  setenv("__UNTAGGED_READ_MODE", "INVALID", 1);
  __update_envar_settings("__UNTAGGED_READ_MODE");
  EXPECT_EQ(__NO_TAG_READ_DEFAULT, __get_no_tag_read_behaviour());
}
