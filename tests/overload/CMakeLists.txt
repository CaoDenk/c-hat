find_package(Catch2 3 REQUIRED)

add_executable(overload_catch2_test OverloadTest.cpp)
target_include_directories(overload_catch2_test PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(overload_catch2_test PRIVATE Catch2::Catch2WithMain lexer ast parser semantic types)
