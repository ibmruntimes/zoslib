#!/usr/bin/env bash
###############################################################################
# Licensed Materials - Property of IBM
# ZOSLIB
# (C) Copyright IBM Corp. 2022. All Rights Reserved.
# US Government Users Restricted Rights - Use, duplication
# or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
###############################################################################

usage() {
  cat <<END
$0
Builds zoslib, uses xlclang/xlclang++ by default, unless CC is set.
Options:
-c    Clean build
-h    Display this message
-r    Release build (default is Debug)
-t    Build and run tests
END
  exit 1
}

SCRIPT_DIR="$( cd "$(dirname "$0")" >/dev/null 2>&1 && pwd -P )"

BLD_TYPE="Debug"
IS_CLEAN=0
RUN_TESTS="OFF"
BLD_TESTS=

if test -z "$CC"; then
  export CC=xlclang && export CXX=xlclang++ && export LINK=xlclang++
  IS_CLEAN=1
fi

nargs=0
while getopts "chrt" o; do
  case "${o}" in
    c) IS_CLEAN=1
       ((nargs++))
       ;;
    r) BLD_TYPE="Release"
       ((nargs++))
       ;;
    t) RUN_TESTS="ON"
       BLD_TESTS="-DBUILD_TESTING=ON"
       ((nargs++))
       ;;
    *) usage
       ;;
  esac
done

# getopts ignores a token if not prefixed with -, e.g.: s instead of -s
[[ "$#" != "$nargs" ]] && usage

[[ "$V" == "1" ]] && set -x
set -e

if((IS_CLEAN==1)); then
  /bin/rm -rf build install
fi
! test -d build && mkdir build

pushd build
export MAKEFLAGS='-j4'

if((IS_CLEAN==1)) || ! test -s CMakeCache.txt; then
  cmake .. -DCMAKE_C_COMPILER=${CC} -DCMAKE_CXX_COMPILER=${CXX} -DCMAKE_ASM_COMPILER=${CC} ${BLD_TESTS} -DCMAKE_BUILD_TYPE=${BLD_TYPE} -DCMAKE_INSTALL_PREFIX=${SCRIPT_DIR}/install
fi
cmake --build . --target install

popd

if [[ "${RUN_TESTS}" == "ON" ]]; then
  export GTEST_OUTPUT="xml:zoslib_a.xml"
  build/test/cctest_a
  build/test/cctest_alnewdel_a

  export LIBPATH="${SCRIPT_DIR}/install/lib:$LIBPATH"
  export GTEST_OUTPUT="xml:zoslib.xml"
  build/test/cctest
fi
