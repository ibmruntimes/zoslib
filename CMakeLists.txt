project(libzoslib)
cmake_minimum_required(VERSION 3.5)

include_directories(BEFORE include)

add_subdirectory(src)

export(TARGETS zoslib FILE lib/cmake/zoslib/addzoslib.cmake)

SET(CMAKE_INSTALL_PREFIX ".")

INSTALL (
    DIRECTORY ${CMAKE_SOURCE_DIR}/include/
    DESTINATION "install/include"
    FILES_MATCHING PATTERN "*.h*")
