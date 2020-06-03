project(libzoslib CXX C ASM)
cmake_minimum_required(VERSION 3.5)

include_directories(BEFORE include)

add_subdirectory(src)

export(TARGETS zoslib FILE lib/cmake/zoslib/addzoslib.cmake)

IF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  SET(CMAKE_INSTALL_PREFIX "." CACHE PATH "install path" FORCE)
ENDIF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

INSTALL (
    DIRECTORY ${CMAKE_SOURCE_DIR}/include/
    DESTINATION "install/include"
    FILES_MATCHING PATTERN "*.h*")
