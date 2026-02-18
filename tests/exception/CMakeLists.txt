find_package(Catch2 3 REQUIRED)

add_executable(exception_catch2_test ExceptionTest.cpp)
target_include_directories(exception_catch2_test PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(exception_catch2_test PRIVATE Catch2::Catch2WithMain lexer ast parser semantic types)
