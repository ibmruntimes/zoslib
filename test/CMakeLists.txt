set(gtest_sources gtest_main.cc gtest/gtest-all.cc)
file(GLOB zoslib_test_sources "${CMAKE_CURRENT_SOURCE_DIR}/test-*.cc")

add_executable(cctest ${gtest_sources} ${zoslib_test_sources})
add_executable(cctest_a ${gtest_sources} ${zoslib_test_sources})
add_dependencies(cctest zoslib)
add_dependencies(cctest_a zoslib_a)

target_include_directories(cctest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(cctest PRIVATE ${zoslib_defines})
target_compile_options(cctest PRIVATE ${zoslib_cflags})

target_include_directories(cctest_a PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(cctest_a PRIVATE ${zoslib_defines})
target_compile_options(cctest_a PRIVATE ${zoslib_cflags})

target_link_libraries(cctest_a zoslib_a)
target_link_libraries(cctest ${PROJECT_BINARY_DIR}/lib/libzoslib.x)
