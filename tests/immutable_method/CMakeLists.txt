find_package(Catch2 3 REQUIRED)

add_executable(immutable_method_catch2_test ImmutableMethodTest.cpp)
target_include_directories(immutable_method_catch2_test PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(immutable_method_catch2_test PRIVATE Catch2::Catch2WithMain lexer ast parser semantic types)
