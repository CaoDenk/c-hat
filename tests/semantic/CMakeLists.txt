find_package(Catch2 3 REQUIRED)

add_executable(semantic_catch2_test SemanticTest.cpp)
target_include_directories(semantic_catch2_test PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(semantic_catch2_test PRIVATE Catch2::Catch2WithMain lexer ast parser semantic types)
