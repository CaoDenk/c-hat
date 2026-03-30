find_package(Catch2 3 REQUIRED)
add_executable(module_catch2_test ModuleTest.cpp)
target_include_directories(module_catch2_test PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(module_catch2_test PRIVATE Catch2::Catch2WithMain lexer ast parser semantic types)
