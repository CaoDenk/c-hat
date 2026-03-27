find_package(Catch2 3 REQUIRED)

add_executable(match_catch2_test MatchTest.cpp)
target_include_directories(match_catch2_test PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(match_catch2_test PRIVATE Catch2::Catch2WithMain lexer ast parser semantic types)
