find_package(Catch2 3 REQUIRED)

add_executable(defer_catch2_test DeferTest.cpp)
target_include_directories(defer_catch2_test PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(defer_catch2_test PRIVATE Catch2::Catch2WithMain lexer ast parser semantic types)
