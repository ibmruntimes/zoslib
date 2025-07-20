///////////////////////////////////////////////////////////////////////////////
// Licensed Materials - Property of IBM
// ZOSLIB
// (C) Copyright IBM Corp. 2020, 2024. All Rights Reserved.
// US Government Users Restricted Rights - Use, duplication
// or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
///////////////////////////////////////////////////////////////////////////////

#include "error.h"
#include "gtest/gtest.h"

#include <string.h>
#include <errno.h>

TEST(ErrorTest, Basic) {
    // This is a basic test that calls the error function.
    // The error function should not return, so we use ASSERT_DEATH.
    ASSERT_DEATH(error(1, EINVAL, "Test error"), "Test error: Invalid argument");
}
