add_executable(tuple_catch2_test TupleTest.cpp)
target_include_directories(tuple_catch2_test PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(tuple_catch2_test PRIVATE Catch2::Catch2WithMain lexer ast parser semantic types)
