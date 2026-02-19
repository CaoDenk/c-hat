add_executable(lambda_catch2_test LambdaTest.cpp)
target_link_libraries(lambda_catch2_test PRIVATE lexer parser ast semantic types Catch2::Catch2WithMain)
add_test(NAME lambda_catch2_test COMMAND lambda_catch2_test)
