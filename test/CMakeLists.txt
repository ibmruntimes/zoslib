set(gtest_sources gtest/gtest_main.cc gtest/gtest-all.cc)

add_executable(cctest ${gtest_sources})

target_include_directories(cctest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(cctest PRIVATE ${zoslib_defines})
target_compile_options(cctest PRIVATE ${zoslib_cflags})
target_link_libraries(cctest zoslib)

INSTALL(TARGETS cctest DESTINATION "install/bin")
