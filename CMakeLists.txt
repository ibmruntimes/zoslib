project(libzoslib CXX C ASM)
cmake_minimum_required(VERSION 3.5)

include_directories(BEFORE include)

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
  __static_assert=static_assert
  _UNIX03_SOURCE
  _UNIX03_THREADS
  _UNIX03_WITHDRAWN
  _XOPEN_SOURCE=600
  _XOPEN_SOURCE_EXTENDED)

if(DEFINED ENV{BUILD_VERSION})
  list(APPEND zoslib_defines BUILD_VERSION="$ENV{BUILD_VERSION}")
endif()

list(APPEND zoslib_cflags -Wno-parentheses -Wno-unused-value)

list(APPEND zoslib_cflags -Wc,DLL)

list(APPEND zoslib_cflags
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

target_compile_definitions(zoslib PRIVATE ${zoslib_defines})
target_compile_options(zoslib PRIVATE ${zoslib_cflags})

export(TARGETS zoslib FILE lib/cmake/zoslib/addzoslib.cmake)

IF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  SET(CMAKE_INSTALL_PREFIX "." CACHE PATH "install path" FORCE)
ENDIF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

INSTALL (
    DIRECTORY ${CMAKE_SOURCE_DIR}/include/
    DESTINATION "install/include"
    FILES_MATCHING PATTERN "*.h*")
