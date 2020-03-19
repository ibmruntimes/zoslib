project(libzoslib)
cmake_minimum_required(VERSION 3.5)

include_directories(BEFORE include)

add_subdirectory(src)

export(TARGETS zoslib FILE lib/cmake/zoslib/addzoslib.cmake)
