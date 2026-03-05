add_executable(test_printf test_printf.cpp)
target_include_directories(test_printf PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_printf PRIVATE lexer ast parser semantic types llvm_codegen)

add_executable(variadic_catch2_test VariadicTest.cpp)
target_include_directories(variadic_catch2_test PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(variadic_catch2_test PRIVATE Catch2::Catch2WithMain lexer ast parser semantic types)
