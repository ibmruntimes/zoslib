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
-s    Shared libray build (default is Static)
-t    Build and run tests
END
  exit 1
}

SCRIPT_DIR="$( cd "$(dirname "$0")" >/dev/null 2>&1 && pwd -P )"

BLD_TYPE="Debug"
IS_CLEAN=0
SHARED="OFF"
RUN_TESTS="OFF"
CCTEST="cctest_a"

envcc=$CC && envcxx=$CXX && envlink=$LINK

if [ -f "${SCRIPT_DIR}/build.cache" ]; then
  source "${SCRIPT_DIR}/build.cache"
  if ! test -z "$envcc"; then
    # environment variables CC, CXX, LINK override those in build.cache:
    if [[ "$envcc" != "$CC" ]]; then
      export CC=$envcc && export CXX=$envcxx && export LINK=$envlink
      IS_CLEAN=1
    fi
  fi
elif test -z "$CC"; then
  export CC=xlclang && export CXX=xlclang++ && export LINK=xlclang++
  IS_CLEAN=1
fi

nargs=0
while getopts "chrst" o; do
  case "${o}" in
    c) IS_CLEAN=1
       ((nargs++))
       ;;
    r) BLD_TYPE="Release"
       ((nargs++))
       ;;
    s) SHARED="ON"
       CCTEST=cctest_so
       ((nargs++))
       ;;
    t) RUN_TESTS="ON"
       ((nargs++))
       ;;
    *) usage
       ;;
  esac
done

# getopts ignores a token if not prefixed with -, e.g.: s instead of -s
[[ "$#" != "$nargs" ]] && usage

/bin/cat > build.cache <<EOF
CC=$CC
CXX=$CXX
ASM=$CC
LINK=$CXX
BLD_TYPE=$BLD_TYPE
SHARED=$SHARED
RUN_TESTS=$RUN_TESTS
CCTEST=$CCTEST
EOF

[[ "$V" == "1" ]] && set -x
set -e

if((IS_CLEAN==1)); then
  /bin/rm -rf CMakeFiles CMakeCache.txt
  test -d build && rm -rf build/* || mkdir build
  /bin/rm -rf install
else
  ! test -d build && mkdir build
fi

# make 4.1 doesn't pass the -j down to sub-make, so set it in MAKEFLAGS:
if ! test -z "$NUM_JOBS"; then
  export MAKEFLAGS="-j ${NUM_JOBS} $MAKEFLAGS"
fi

! test -d build && echo "build: directory doesn't exist." && exit -1
pushd build

cmake .. -DCMAKE_C_COMPILER=${CC} -DCMAKE_CXX_COMPILER=${CXX} -DCMAKE_ASM_COMPILER=${CC} -DBUILD_TESTING=ON -DCMAKE_BUILD_TYPE=${BLD_TYPE} -DBUILD_SHARED_LIBS=${SHARED}
cmake --build . --target install

! test -d ../install && mv install .. || /bin/cp -R install/* ../install/ && /bin/rm -rf install

popd

if [[ "${RUN_TESTS}" == "ON" ]]; then
  export GTEST_OUTPUT="xml:zoslib.xml"
  [[ "$SHARED" == "ON" ]] && export LIBPATH=`pwd`/install/lib:$LIBPATH
  install/bin/$CCTEST
fi
