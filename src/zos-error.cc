///////////////////////////////////////////////////////////////////////////////
// Licensed Materials - Property of IBM
// ZOSLIB
// (C) Copyright IBM Corp. 2020, 2024. All Rights Reserved.
// US Government Users Restricted Rights - Use, duplication
// or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
///////////////////////////////////////////////////////////////////////////////

#include "error.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

extern "C" void __error(int status, int errnum, const char *format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    if (errnum != 0) {
      fprintf(stderr, ": %s\n", strerror(errnum));
    }
    if (status)
      exit(status);
}
