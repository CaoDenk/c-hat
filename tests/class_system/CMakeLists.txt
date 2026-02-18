find_package(Catch2 3 REQUIRED)

add_executable(class_catch2_test ClassTest.cpp)
target_include_directories(class_catch2_test PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(class_catch2_test PRIVATE Catch2::Catch2WithMain lexer ast parser semantic types)
