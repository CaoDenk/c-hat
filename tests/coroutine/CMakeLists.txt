find_package(Catch2 3 REQUIRED)

add_executable(coroutine_catch2_test CoroutineTest.cpp)
target_include_directories(coroutine_catch2_test PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(coroutine_catch2_test PRIVATE Catch2::Catch2WithMain lexer ast parser semantic types)
