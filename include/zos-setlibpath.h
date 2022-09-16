///////////////////////////////////////////////////////////////////////////////
// Licensed Materials - Property of IBM
// ZOSLIB
// (C) Copyright IBM Corp. 2021. All Rights Reserved.
// US Government Users Restricted Rights - Use, duplication
// or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
///////////////////////////////////////////////////////////////////////////////

#ifndef ZOS_SETLIBPATH_H_
#define ZOS_SETLIBPATH_H_

#include "zos-macros.h"

#include <libgen.h>
#include <sstream>
#include <sys/ps.h>

class __Z_EXPORT __setlibpath {
public:
  __setlibpath() {
    std::vector<char> argv(512, 0);
    std::vector<char> parent(512, 0);
    W_PSPROC buf;
    int token = 0;
    pid_t mypid = getpid();
    memset(&buf, 0, sizeof(buf));
    buf.ps_pathlen = argv.size();
    buf.ps_pathptr = &argv[0];
    while ((token = w_getpsent(token, &buf, sizeof(buf))) > 0) {
      if (buf.ps_pid == mypid) {
        /* Found our process. */

        /* Resolve path to find true location of executable. */
        if (realpath(&argv[0], &parent[0]) == NULL)
          break;

        /* Get parent directory. */
        dirname(&parent[0]);

        /* Get parent's parent directory. */
        std::vector<char> parent2(parent.begin(), parent.end());
        dirname(&parent2[0]);

        /* Append new paths to libpath. */
        std::ostringstream libpath;
        libpath << getenv("LIBPATH");
        libpath << ":" << &parent[0] << "/lib.target/";
        libpath << ":" << &parent[0] << "/lib/";
        libpath << ":" << &parent2[0] << "/lib/";
        setenv("LIBPATH", libpath.str().c_str(), 1);
        break;
      }
    }
  }
};

#endif // ZOS_SETLIBPATH_H_
