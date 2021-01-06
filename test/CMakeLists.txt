set(gtest_sources gtest_main.cc gtest/gtest-all.cc)
file(GLOB zoslib_test_sources "${CMAKE_CURRENT_SOURCE_DIR}/test-*.cc")

add_executable(cctest ${gtest_sources} ${zoslib_test_sources})

target_include_directories(cctest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(cctest PRIVATE ${zoslib_defines})
target_compile_options(cctest PRIVATE ${zoslib_cflags})
target_link_libraries(cctest zoslib)

INSTALL(TARGETS cctest DESTINATION "install/bin")
