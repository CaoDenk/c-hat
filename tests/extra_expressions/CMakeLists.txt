find_package(Catch2 3 REQUIRED)
add_executable(extra_expr_catch2_test ExtraExprTest.cpp)
target_include_directories(extra_expr_catch2_test PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(extra_expr_catch2_test PRIVATE Catch2::Catch2WithMain lexer ast parser semantic types)
