find_package(Catch2 3 REQUIRED)

add_executable(late_catch2_test LateTest.cpp)
target_include_directories(late_catch2_test PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(late_catch2_test PRIVATE Catch2::Catch2WithMain lexer ast parser semantic types)
