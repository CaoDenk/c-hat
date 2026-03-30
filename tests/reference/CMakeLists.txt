find_package(Catch2 3 REQUIRED)
add_executable(reference_catch2_test ReferenceTest.cpp)
target_include_directories(reference_catch2_test PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(reference_catch2_test PRIVATE Catch2::Catch2WithMain lexer ast parser semantic types)
