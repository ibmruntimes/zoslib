#include "zos.h"
#include "gtest/gtest.h"
#include <time.h>

/*
TEST(TimegmTest, EpochStart) {
    struct tm t = {};
    t.tm_year = 70;  // 1970
    t.tm_mon = 0;    // January
    t.tm_mday = 1;   // 1st
    t.tm_hour = 0;
    t.tm_min = 0;
    t.tm_sec = 0;
    EXPECT_EQ(timegm(&t), 0);
}

TEST(TimegmTest, RandomDate) {
    struct tm t = {};
    t.tm_year = 121; // 2021
    t.tm_mon = 6;    // July
    t.tm_mday = 20;
    t.tm_hour = 15;
    t.tm_min = 45;
    t.tm_sec = 30;

    time_t ts = timegm(&t);
    // 2021-07-20 15:45:30 UTC should be 1626795930
    EXPECT_EQ(ts, 1626795930);
}

TEST(TimegmTest, LeapYearFeb29) {
    struct tm t = {};
    t.tm_year = 104; // 2004
    t.tm_mon = 1;    // February
    t.tm_mday = 29;
    t.tm_hour = 12;
    EXPECT_EQ(timegm(&t), 1078065600);
}

TEST(TimegmTest, BeforeEpoch) {
    struct tm t = {};
    t.tm_year = 60; // 1960
    t.tm_mon = 5;   // June
    t.tm_mday = 15;
    EXPECT_EQ(timegm(&t), -306720000);
}

TEST(TimegmTest, EndOfYear) {
    struct tm t = {};
    t.tm_year = 122; // 2022
    t.tm_mon = 11;   // December
    t.tm_mday = 31;
    t.tm_hour = 23;
    t.tm_min = 59;
    t.tm_sec = 59;
    EXPECT_EQ(timegm(&t), 1672531199);
}
*/

TEST(TimegmTest, NormalDate) {
    struct tm t = {};
    t.tm_year = 2020 - 1900; // 2020
    t.tm_mon  = 0;           // January
    t.tm_mday = 1;           // 1st
    t.tm_hour = 0;
    t.tm_min  = 0;
    t.tm_sec  = 0;

    EXPECT_EQ(timegm(&t), 1577836800); // 2020-01-01 00:00:00 UTC
}

TEST(TimegmTest, LeapYearFeb29) {
    struct tm t = {};
    t.tm_year = 2004 - 1900; // 2004
    t.tm_mon  = 1;           // February
    t.tm_mday = 29;          // 29th
    t.tm_hour = 0;
    t.tm_min  = 0;
    t.tm_sec  = 0;

    EXPECT_EQ(timegm(&t), 1078012800); // 2004-02-29 00:00:00 UTC
}

TEST(TimegmTest, NonLeapYearFeb28) {
    struct tm t = {};
    t.tm_year = 2001 - 1900; // 2001
    t.tm_mon  = 1;           // February
    t.tm_mday = 28;          // 28th
    t.tm_hour = 23;
    t.tm_min  = 59;
    t.tm_sec  = 59;

    EXPECT_EQ(timegm(&t), 983404799); // 2001-02-28 23:59:59 UTC
}

TEST(TimegmTest, EndOfYear) {
    struct tm t = {};
    t.tm_year = 1999 - 1900; // 1999
    t.tm_mon  = 11;          // December
    t.tm_mday = 31;
    t.tm_hour = 23;
    t.tm_min  = 59;
    t.tm_sec  = 59;

    EXPECT_EQ(timegm(&t), 946684799); // 1999-12-31 23:59:59 UTC
}

TEST(TimegmTest, StartOfEpoch) {
    struct tm t = {};
    t.tm_year = 1970 - 1900; // 1970
    t.tm_mon  = 0;           // January
    t.tm_mday = 1;
    t.tm_hour = 0;
    t.tm_min  = 0;
    t.tm_sec  = 0;

    EXPECT_EQ(timegm(&t), 0); // Epoch start
}
