set(gtest_sources gtest_main.cc gtest/gtest-all.cc)
file(GLOB zoslib_test_sources "${CMAKE_CURRENT_SOURCE_DIR}/test-*.cc")
file(GLOB zoslib_test_alnewdel_sources "${CMAKE_CURRENT_SOURCE_DIR}/alnewdel/test-*.cc")

add_executable(cctest ${gtest_sources} ${zoslib_test_sources})
add_executable(cctest_a ${gtest_sources} ${zoslib_test_sources})
add_executable(cctest_alnewdel_a ${gtest_sources} ${zoslib_test_alnewdel_sources})

add_dependencies(cctest zoslib)
add_dependencies(cctest_a zoslib_a)
add_dependencies(cctest_alnewdel_a zoslib_alnewdel_a zoslib_a)

target_include_directories(cctest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(cctest PRIVATE ${zoslib_defines})
target_compile_options(cctest PRIVATE ${zoslib_cflags})

target_include_directories(cctest_a PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(cctest_a PRIVATE ${zoslib_defines})
target_compile_options(cctest_a PRIVATE ${zoslib_cflags})

target_include_directories(cctest_alnewdel_a PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/include-wrappers/c++)
target_compile_definitions(cctest_alnewdel_a PRIVATE ${zoslib_defines} ${zoslib_alnewdel_defines})
target_compile_options(cctest_alnewdel_a PRIVATE ${zoslib_cflags} -faligned-allocation)

target_link_libraries(cctest_a zoslib_a)
target_link_libraries(cctest_alnewdel_a zoslib_alnewdel_a zoslib_a)
target_link_libraries(cctest ${PROJECT_BINARY_DIR}/lib/libzoslib.x)
