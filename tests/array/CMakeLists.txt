find_package(Catch2 3 REQUIRED)

add_executable(array_catch2_test ArrayTest.cpp)
target_include_directories(array_catch2_test PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(array_catch2_test PRIVATE Catch2::Catch2WithMain lexer ast parser semantic types)
