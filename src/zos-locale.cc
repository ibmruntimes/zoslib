///////////////////////////////////////////////////////////////////////////////
// Licensed Materials - Property of IBM
// ZOSLIB
// (C) Copyright IBM Corp. 2024. All Rights Reserved.
// US Government Users Restricted Rights - Use, duplication
// or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
///////////////////////////////////////////////////////////////////////////////

#ifndef __ibmxl__
// libc++ in xlclang++ leaks newlocale/locale_t so zoslib can't provide 
// alternatives for that compiler
#ifndef ZOSLIB_OVERRIDE_CLIB_LOCALE
#define ZOSLIB_OVERRIDE_CLIB_LOCALE
#endif
#include <locale.h>
#include <stdlib.h>

#include <errno.h>
#include <string>

struct locale_struct {
  int category_mask;
  std::string lc_collate;
  std::string lc_ctype;
  std::string lc_monetary;
  std::string lc_numeric;
  std::string lc_time;
  std::string lc_messages;
};

#define CategoryList(pair, sep) \
  pair(LC_COLLATE, lc_collate) sep \
  pair(LC_CTYPE, lc_ctype) sep \
  pair(LC_MONETARY, lc_monetary) sep \
  pair(LC_NUMERIC, lc_numeric) sep \
  pair(LC_TIME, lc_time) sep \
  pair(LC_MESSAGES, lc_messages)

// check ids and masks agree
#define check_ids_and_masks_agree(id, _) \
  static_assert((1 << id) == id##_MASK, "id and mask do not agree for " #id); \
  static_assert((1 << id) == _CATMASK(id), "mask does not have expected value for " #id);
CategoryList(check_ids_and_masks_agree,)
#undef check_ids_and_masks_agree

// check that LC_ALL_MASK is defined as expected
#define get_mask(id, _) id##_MASK
static_assert(LC_ALL_MASK == (CategoryList(get_mask, |)), "LC_ALL_MASK does not have the expected value.  Check that its definition includes all supported categories");
#undef get_mask


// initialize c_locale
#define init_clocale(id, locale_member) "C",
static locale_struct c_locale = { LC_ALL_MASK, CategoryList(init_clocale, ) };
#undef init_clocale

static locale_t current_locale = LC_GLOBAL_LOCALE;


extern "C" {

locale_t __c_locale() {
  return &c_locale;
}

// locale
locale_t newlocale(int category_mask, const char* locale, locale_t base) {
  // start with some basic checks
  if (! locale) {
    errno = EINVAL;
    return (locale_t) 0;
  }
  if (category_mask & ~LC_ALL_MASK) {
    // then there are bits in category_mask that does not correspond
    // to a valid category
    errno = EINVAL;
    return (locale_t) 0;
  }

  locale_t  new_loc = new locale_struct;
  int num_locales_not_found = 0;

  if (base && base != LC_GLOBAL_LOCALE)
    *new_loc = *base;

  auto set_for_category = [&](int id, int mask, std::string &setting) {
    const char *setting_to_apply = nullptr;

    if (category_mask & mask)
      setting_to_apply = locale;
    else if (!base)
      setting_to_apply = "C";

    if (setting_to_apply) {
      // setlocale takes the id, not the mask
      const char *saved_setting = setlocale(id, nullptr);
      if (setlocale(id, setting_to_apply)) {
        // then setting_to_apply is valid for this category
        // restore the saved setting
        setlocale(id, saved_setting);

        new_loc->category_mask |= mask;
        setting = setting_to_apply;
      } else {
        // unknown locale for this category
        num_locales_not_found++;
      }
    }
  };

  // now overlay values
#define Set(id, locale_member) set_for_category(id, id##_MASK, new_loc->locale_member)
  CategoryList(Set, ;);
#undef Set

  if (num_locales_not_found != 0) {
    delete new_loc;
    errno = ENOENT;
    new_loc = (locale_t) 0;
  }

  return new_loc;
}

void freelocale(locale_t locobj) {
  if (locobj != nullptr && locobj != &c_locale && locobj != LC_GLOBAL_LOCALE)
    delete locobj;
}

locale_t uselocale(locale_t new_loc) {
  locale_t prev_loc = current_locale;

  if (new_loc == LC_GLOBAL_LOCALE) {
    current_locale = LC_GLOBAL_LOCALE;
  } else if (new_loc != nullptr) {
    locale_struct  saved_locale;
    saved_locale.category_mask = 0;

    auto apply_category = [&](int id, int mask, std::string &setting, std::string &save_setting)-> bool {
      if (new_loc->category_mask & mask) {
        const char *old_setting = setlocale(id, setting.c_str());
        if (old_setting) {
          // we were able to set for this category.  Save the old setting
          // in case a subsequent category fails, and we need to restore
          // all earlier settings.
          saved_locale.category_mask |= mask;
          save_setting = old_setting;
          return true;
        }
        return false;
      }
      return true;
    };

#define Apply(id, locale_member) apply_category(id, id##_MASK, new_loc->locale_member, saved_locale. locale_member)
    bool is_ok =
  CategoryList(Apply, &&);
#undef Apply

    if (!is_ok) {
      auto restore = [&](int id, int mask, std::string &setting) {
        if (saved_locale.category_mask & mask)
          setlocale(id, setting.c_str());
      };
#define Restore(id, locale_member) restore(id, id##_MASK, saved_locale. locale_member);
      CategoryList(Restore,);
#undef Restore
      errno = EINVAL;
      return nullptr;
    }
    current_locale = new_loc;
  }

  return prev_loc;
}

struct SetAndRestore {
  explicit SetAndRestore(locale_t l) {
    if (l == (locale_t)0) {
      _stored = uselocale(&c_locale);
    } else {
      _stored = uselocale(l);
    }
  }

  ~SetAndRestore() {
    uselocale(_stored);
  }

  private:
    locale_t _stored = (locale_t)0;
};

double strtod_l(const char * __restrict__ str, char ** __restrict__ end, locale_t l) {
  SetAndRestore newloc(l);
  return strtod(str, end);
}
}
#endif
