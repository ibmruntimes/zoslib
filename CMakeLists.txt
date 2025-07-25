###############################################################################
# Licensed Materials - Property of IBM
# ZOSLIB
# (C) Copyright IBM Corp. 2020. All Rights Reserved.
# US Government Users Restricted Rights - Use, duplication
# or disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
###############################################################################

cmake_minimum_required(VERSION 3.24)
project(libzoslib CXX C ASM)

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
set(CMAKE_EXE_LINKER_FLAGS "-m64" CACHE STRING "" FORCE)
set(CMAKE_SHARED_LINKER_FLAGS "-m64" CACHE STRING "" FORCE)

list(APPEND zoslib_defines
  _AE_BIMODAL=1
  _ALL_SOURCE
  _ENHANCED_ASCII_EXT=0xFFFFFFFF
  _LARGE_TIME_API
  _OPEN_MSGQ_EXT
  _OPEN_SYS_FILE_EXT=1
  _OPEN_SYS_SOCK_IPV6
  PATH_MAX=1024
  _UNIX03_SOURCE
  _UNIX03_THREADS
  _UNIX03_WITHDRAWN
  _XOPEN_SOURCE=600
  _XOPEN_SOURCE_EXTENDED
)

list(APPEND zoslib_alnewdel_defines
  ZOSLIB_ALIGNED_NEWDEL
)

if(ENABLE_STATIC_INIT)
  list(APPEND zoslib_defines
       ZOSLIB_INITIALIZE)
endif()

if(ZOSLIB_GENERIC)
  list(APPEND zoslib_defines
       ZOSLIB_GENERIC)
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
    -qasmlib=sys1.maclib:sys1.modgen
  )

  target_link_options(zoslib PUBLIC "-Wl,DLL")
else()
  # min arch14 is required for zos-getentropy.cc to use PRNO instruction.
  list(APPEND zoslib_cflags
    -fgnu-keywords
    -fno-short-enums
    -fzos-le-char-mode=ascii
    -m64
    -march=arch14
    -mzos-target=zosv2r4
  )

  target_link_options(zoslib PUBLIC "--shared")
endif()

target_compile_definitions(libzoslib PRIVATE ${zoslib_defines})
target_compile_options(libzoslib PRIVATE ${zoslib_cflags})

target_compile_definitions(libzoslib_alnewdel PRIVATE ${zoslib_defines} ${zoslib_alnewdel_defines})
target_compile_options(libzoslib_alnewdel PRIVATE ${zoslib_cflags})
target_include_directories(libzoslib_alnewdel PRIVATE ${CMAKE_SOURCE_DIR}/include/c++/v1)

target_compile_definitions(zoslib-help PRIVATE ${zoslib_defines})
target_compile_options(zoslib-help PRIVATE ${zoslib_cflags})

if(BUILD_TESTING)
  add_subdirectory(test)
endif()

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  SET(CMAKE_INSTALL_PREFIX "." CACHE PATH "install path" FORCE)
endif(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
