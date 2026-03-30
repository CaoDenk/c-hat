find_package(Catch2 3 REQUIRED)
add_executable(string_literal_catch2_test StringLiteralTest.cpp)
target_include_directories(string_literal_catch2_test PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(string_literal_catch2_test PRIVATE Catch2::Catch2WithMain lexer ast parser semantic types)
