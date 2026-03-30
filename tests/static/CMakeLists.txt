find_package(Catch2 3 REQUIRED)
add_executable(static_catch2_test StaticTest.cpp)
target_include_directories(static_catch2_test PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(static_catch2_test PRIVATE Catch2::Catch2WithMain lexer ast parser semantic types)
