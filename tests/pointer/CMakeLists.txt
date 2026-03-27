find_package(Catch2 3 REQUIRED)

add_executable(pointer_catch2_test PointerTest.cpp)
target_include_directories(pointer_catch2_test PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(pointer_catch2_test PRIVATE Catch2::Catch2WithMain lexer ast parser semantic types)
