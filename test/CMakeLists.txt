set(gtest_sources gtest_main.cc gtest/gtest-all.cc)
file(GLOB zoslib_test_sources "${CMAKE_CURRENT_SOURCE_DIR}/test-*.cc")

add_executable(cctest ${gtest_sources} ${zoslib_test_sources})
add_dependencies(cctest zoslib)

if((NOT BUILD_SHARED_LIBS) OR (${BUILD_SHARED_LIBS} MATCHES "OFF"))
  set_target_properties(cctest PROPERTIES OUTPUT_NAME "cctest_a")
else()
  set_target_properties(cctest PROPERTIES OUTPUT_NAME "cctest_so")
endif()

target_include_directories(cctest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(cctest PRIVATE ${zoslib_defines})
target_compile_options(cctest PRIVATE ${zoslib_cflags})
if((NOT BUILD_SHARED_LIBS) OR (${BUILD_SHARED_LIBS} MATCHES "OFF"))
  target_link_libraries(cctest zoslib)
else()
  target_link_libraries(cctest ${CMAKE_BINARY_DIR}/lib/libzoslib.x)
endif()

INSTALL(TARGETS cctest DESTINATION "install/bin")
