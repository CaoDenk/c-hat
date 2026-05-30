file(GLOB TEST_SOURCES "*.cpp")

add_executable(reflection_catch2_test ${TEST_SOURCES})
target_link_libraries(reflection_catch2_test
    Catch2::Catch2WithMain
    parser
    semantic
    llvm_codegen
    types
    ast
    code_generator
    lexer
)
target_include_directories(reflection_catch2_test PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

include(Catch)
catch_discover_tests(reflection_catch2_test)
