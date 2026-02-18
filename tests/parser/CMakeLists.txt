find_package(Catch2 3 REQUIRED)

add_executable(parser_catch2_test ParserTest.cpp)
target_include_directories(parser_catch2_test PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(parser_catch2_test PRIVATE Catch2::Catch2WithMain lexer ast parser types)
