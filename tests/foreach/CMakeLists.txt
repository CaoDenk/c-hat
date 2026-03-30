find_package(Catch2 3 REQUIRED)
add_executable(foreach_catch2_test ForeachTest.cpp)
target_include_directories(foreach_catch2_test PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(foreach_catch2_test PRIVATE Catch2::Catch2WithMain lexer ast parser semantic types)
