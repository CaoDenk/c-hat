find_package(Catch2 3 REQUIRED)
add_executable(generics_catch2_test GenericsTest.cpp)
target_include_directories(generics_catch2_test PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(generics_catch2_test PRIVATE Catch2::Catch2WithMain lexer ast parser semantic types)
