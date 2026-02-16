find_package(Catch2 3 REQUIRED)

add_executable(parser_test parser_test.cpp)
target_include_directories(parser_test PRIVATE ../src)
target_link_libraries(parser_test PRIVATE Catch2::Catch2WithMain lexer ast parser types)

add_executable(semantic_test semantic_test.cpp)
target_include_directories(semantic_test PRIVATE ../src)
target_link_libraries(semantic_test PRIVATE lexer ast parser semantic types)
