//////////////////////////////////////////////////////////////////////////////
//// Licensed Materials - Property of IBM
//// ZOSLIB
//// (C) Copyright IBM Corp. 2020. All Rights Reserved.
//// US Government Users Restricted Rights - Use, duplication
//// or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
/////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>

typedef enum {
  cp_ebcdic,
  cp_ascii
} CodePage;

#define XXXXXX "XXXXXX"
#define ValidChars "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_.,-"
static const struct {
  char xes[sizeof(XXXXXX)] = XXXXXX;
  char validChars[sizeof(ValidChars)] = ValidChars;
}  CharTraits[] = {
#pragma convert("IBM-1047")
   { Xes,
     ValidChars
   },
#pragma convert(pop)

#pragma convert("ISO8859-1")
   { Xes,
     ValidChars
   }
#pragma convert(pop)
};

enum {
  TempLen = sizeof(Xes);
};

char *real_mkdtemp(CodePage cp, char *templ) {
  if (!templ) {
    errno=EINVAL;
    return NULL;
  }
  int len = strlen(templ);

  if (len <TempLen || strncmp(templ+len-TempLen,CharTraits[cp].xes ,TempLen)!=0) {
    errno=EINVAL;
    return NULL;
  }
#define validChars CharTraits[cp].validChars
  srand(time(NULL));
#define RANDOM_INT(max) (1 + rand()/((RAND_MAX + 1u) / (max) ))
#define UPDATE_CHAR(n) templ[len-n+n-1] = validChars[(s##n+c##n)%(sizeof validChars)]

  assert(TempLen == 6);
  int s1 = RANDOM_INT(sizeof validChars);
  int s2 = RANDOM_INT(sizeof validChars);
  int s3 = RANDOM_INT(sizeof validChars);
  int s4 = RANDOM_INT(sizeof validChars);
  int s5 = RANDOM_INT(sizeof validChars);
  int s6 = RANDOM_INT(sizeof validChars);
  for (int c1=0; c1<sizeof validChars; ++c1) {
    UPDATE_CHAR(1);
    for (int c2=0; c2<sizeof validChars; ++c2) {
      UPDATE_CHAR(2);
      for (int c3=0; c3<sizeof validChars; ++c3) {
        UPDATE_CHAR(3);
        for (int c4=0; c4<sizeof validChars; ++c4) {
          UPDATE_CHAR(4);
          for (int c5=0; c5<sizeof validChars; ++c5) {
            UPDATE_CHAR(5);
            for (int c6=0; c6<sizeof validChars; ++c6) {
              UPDATE_CHAR(6);
              int rc = mkdir(templ, 0700);
              if (rc==0)
                return templ;
              if (errno!=EEXIST) {
                // restore the X's in the template
                strncpy(templ+len-TempLen,CharTraits[cp].xes ,TempLen);
                return NULL;
              }
            }
          }
        }
      }
    }
  }
  // restore the X's in the template
  strncpy(templ+len-TempLen,CharTraits[cp].xes,TempLen);
  return NULL;
}

__Z_EXPORT char *__mkdtemp_a(char *templ) {
  return real_mkdtemp(cp_ascii, templ);
}

__Z_EXPORT char *__mkdtemp_e(char *templ) {
  return real_mkdtemp(cp_ebcdic, templ);
}
#endif
