find_package(Catch2 3 REQUIRED)

add_executable(new_delete_catch2_test NewDeleteTest.cpp)
target_include_directories(new_delete_catch2_test PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(new_delete_catch2_test PRIVATE Catch2::Catch2WithMain lexer ast parser semantic types)
