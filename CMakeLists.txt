###############################################################################
# Licensed Materials - Property of IBM
# ZOSLIB
# (C) Copyright IBM Corp. 2020. All Rights Reserved.
# US Government Users Restricted Rights - Use, duplication
# or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
###############################################################################

cmake_minimum_required(VERSION 3.24)
project(libzoslib VERSION 4.0.0 LANGUAGES CXX C ASM)

include_directories(BEFORE include)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)

add_subdirectory(src)
add_subdirectory(man)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()

set(CMAKE_CXX_FLAGS_DEBUG "-g" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS_RELEASE "-O3" CACHE STRING "" FORCE)

list(APPEND zoslib_defines
  _AE_BIMODAL=1
  _ALL_SOURCE
  _ENHANCED_ASCII_EXT=0xFFFFFFFF
  _LARGE_TIME_API
  _OPEN_MSGQ_EXT
  _OPEN_SYS_FILE_EXT=1
  _OPEN_SYS_SOCK_IPV6
  _Packed=
  PATH_MAX=1024
  _UNIX03_SOURCE
  _UNIX03_THREADS
  _UNIX03_WITHDRAWN
  _XOPEN_SOURCE=600
  _XOPEN_SOURCE_EXTENDED)

if(ENABLE_STATIC_INIT)
  list(APPEND zoslib_defines
       ZOSLIB_INITIALIZE)
endif()

if(ZOSLIB_GENERIC)
  list(APPEND zoslib_defines
       ZOSLIB_GENERIC)
endif()

if(ZOSLIB_QUICK_STARTUP)
  list(APPEND zoslib_defines
       ZOSLIB_QUICK_STARTUP)
endif()

if(DEFINED ENV{BUILD_VERSION})
  list(APPEND zoslib_defines BUILD_VERSION="$ENV{BUILD_VERSION}")
endif()

list(APPEND zoslib_cflags -Wno-parentheses -Wno-unused-value)

if(${CMAKE_C_COMPILER} MATCHES xlclang)
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

  target_link_options(zoslib PUBLIC "-Wl,DLL")
else()
  # min arch14 is required for zos-getentropy.cc to use PRNO instruction.
  list(APPEND zoslib_cflags
    -fgnu-keywords
    -m64
    -march=z13
    -mzos-target=zosv2r5
    -fno-short-enums
    -fzos-le-char-mode=ascii)

  target_link_options(zoslib PUBLIC "--shared")
endif()

target_compile_definitions(libzoslib PRIVATE ${zoslib_defines})
target_compile_options(libzoslib PRIVATE ${zoslib_cflags})

target_compile_definitions(zoslib-help PRIVATE ${zoslib_defines})
target_compile_options(zoslib-help PRIVATE ${zoslib_cflags})

if(BUILD_TESTING)
  add_subdirectory(test)
endif()

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  SET(CMAKE_INSTALL_PREFIX "." CACHE PATH "install path" FORCE)
endif(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

# Configure and install pkgconfig file
set(ZOSLIB_VERSION "${PROJECT_VERSION}")
# Allow users to specify extra flags and libs via CMake variables
if(NOT DEFINED ZOSLIB_EXTRA_CFLAGS)
  set(ZOSLIB_EXTRA_CFLAGS "")
endif()
if(NOT DEFINED ZOSLIB_EXTRA_LIBS)
  set(ZOSLIB_EXTRA_LIBS "")
endif()
if(NOT DEFINED ZOSLIB_PRIVATE_LIBS)
  set(ZOSLIB_PRIVATE_LIBS "")
endif()
configure_file(
  ${CMAKE_SOURCE_DIR}/zoslib.pc.in
  ${CMAKE_BINARY_DIR}/zoslib.pc
  @ONLY
)
install(
  FILES ${CMAKE_BINARY_DIR}/zoslib.pc
  DESTINATION lib/pkgconfig
)
