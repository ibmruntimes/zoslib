#define ZOSLIB_OVERRIDE_CLIB 1

#include "zos.h"
#include <gtest/gtest.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <locale.h>

class LocaleTest : public ::testing::Test 
{
};

TEST(LocaleTest, SetLocale_Global) {
    const char* orig = setlocale(LC_ALL, nullptr);
    ASSERT_NE(orig, nullptr);

    const char* result = setlocale(LC_ALL, "C");
    ASSERT_STREQ(result, "C");

    // Restore
    setlocale(LC_ALL, orig);
}

TEST(LocaleTest, NewLocale_CreateAndFree) {
    locale_t loc = newlocale(LC_ALL_MASK, "C", nullptr);
    ASSERT_NE(loc, (locale_t)0);

    freelocale(loc); // No crash
}

TEST(LocaleTest, UseLocale_OverridesGlobalTemporarily) {
    const char* global = setlocale(LC_NUMERIC, "C");
    ASSERT_STREQ(global, "C");

    locale_t fr = newlocale(LC_NUMERIC_MASK, "fr_FR.UTF-8", nullptr);
    ASSERT_NE(fr, (locale_t)0);

    locale_t old = uselocale(fr);
    ASSERT_NE(old, (locale_t)0);

    // Localeconv in fr_FR.UTF-8 uses comma
    struct lconv* conv = localeconv();
    ASSERT_STREQ(conv->decimal_point, ",");

    uselocale(old);
    freelocale(fr);
}

TEST(LocaleTest, TestNames) {
    locale_t loc = newlocale(LC_ALL_MASK, "en_US.UTF-8", NULL);
    ASSERT_NE(loc, (locale_t)0) << "Failed to create en_US.UTF-8 locale";

    loc = newlocale(LC_NUMERIC_MASK, "fr_FR.UTF-8", loc);
    ASSERT_NE(loc, (locale_t)0) << "Failed to set LC_NUMERIC locale";

    loc = newlocale(LC_TIME_MASK, "de_DE.UTF-8", loc);
    ASSERT_NE(loc, (locale_t)0) << "Failed to set LC_TIME locale";

    loc = newlocale(LC_MONETARY_MASK, "ja_JP.UTF-8", loc);
    ASSERT_NE(loc, (locale_t)0) << "Failed to set LC_MONETARY locale";

    loc = newlocale(LC_MESSAGES_MASK, "es_ES.UTF-8", loc);
    ASSERT_NE(loc, (locale_t)0) << "Failed to set LC_MESSAGES locale";

    // Assert locale names for each category
    const char* ctype = getlocalename_l(LC_CTYPE, loc);
    ASSERT_STRNE(ctype, "") << "LC_CTYPE should not be empty";
    ASSERT_STREQ(ctype, "en_US.UTF-8");

    const char* numeric = getlocalename_l(LC_NUMERIC, loc);
    ASSERT_STRNE(numeric, "") << "LC_NUMERIC should not be empty";
    ASSERT_STREQ(numeric, "fr_FR.UTF-8");

    const char* time = getlocalename_l(LC_TIME, loc);
    ASSERT_STRNE(time, "") << "LC_TIME should not be empty";
    ASSERT_STREQ(time, "de_DE.UTF-8");

    const char* monetary = getlocalename_l(LC_MONETARY, loc);
    ASSERT_STRNE(monetary, "") << "LC_MONETARY should not be empty";
    ASSERT_STREQ(monetary, "ja_JP.UTF-8");

    const char* messages = getlocalename_l(LC_MESSAGES, loc);
    ASSERT_STRNE(messages, "") << "LC_MESSAGES should not be empty";
    ASSERT_STREQ(messages, "es_ES.UTF-8");

    freelocale(loc);
}

