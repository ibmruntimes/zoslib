###############################################################################
# Licensed Materials - Property of IBM
# ZOSLIB
# (C) Copyright IBM Corp. 2020. All Rights Reserved.
# US Government Users Restricted Rights - Use, duplication
# or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
###############################################################################

project(libzoslib CXX C ASM)
cmake_minimum_required(VERSION 3.5)

include_directories(BEFORE include)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)

add_subdirectory(src)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()

set(CMAKE_CXX_FLAGS_DEBUG "-g" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS_RELEASE "-O3" CACHE STRING "" FORCE)

list(APPEND zoslib_defines
  _AE_BIMODAL=1
  _ALL_SOURCE
  _ENHANCED_ASCII_EXT=0xFFFFFFFF
  _Export=extern
  _LARGE_TIME_API
  _OPEN_MSGQ_EXT
  _OPEN_SYS_FILE_EXT=1
  _OPEN_SYS_SOCK_IPV6
  PATH_MAX=1024
  _UNIX03_SOURCE
  _UNIX03_THREADS
  _UNIX03_WITHDRAWN
  _XOPEN_SOURCE=600
  _XOPEN_SOURCE_EXTENDED)

if(DEFINED ENV{BUILD_VERSION})
  list(APPEND zoslib_defines BUILD_VERSION="$ENV{BUILD_VERSION}")
endif()

list(APPEND zoslib_cflags -Wno-parentheses -Wno-unused-value)

if(${CMAKE_C_COMPILER} MATCHES ibm-clang64)
  list(APPEND zoslib_cflags
    -fgnu-keywords
    -march=arch10
    -mtune=arch10
    -fno-short-enums
    -fzos-le-char-mode=ascii)
else()
  list(APPEND zoslib_cflags
    -fgnu-keywords
    -q64
    -qascii
    -qexportall
    -qgonumber
    -qlongname
    -qlibansi
    -qfloat=IEEE
    -qtune=10
    -qarch=10
    -qasm
    -qasmlib=sys1.maclib:sys1.modgen)
endif()

set(zoslib_cflags ${zoslib_cflags} ${CMAKE_CXX_FLAGS})

target_compile_definitions(libzoslib PRIVATE ${zoslib_defines})
target_compile_options(libzoslib PRIVATE ${zoslib_cflags})

if(BUILD_TESTING)
  add_subdirectory(test)
endif()

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  SET(CMAKE_INSTALL_PREFIX "." CACHE PATH "install path" FORCE)
endif(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
