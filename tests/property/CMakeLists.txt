cmake_minimum_required(VERSION 3.20)
project(property_test)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Catch2 3 REQUIRED)

add_executable(property_catch2_test PropertyTest.cpp)

target_include_directories(property_catch2_test PRIVATE
    ${CMAKE_SOURCE_DIR}/../src
)

target_link_libraries(property_catch2_test PRIVATE
    Catch2::Catch2WithMain
    parser
    semantic
    ast
    types
    llvm_codegen
)

if(MSVC)
    target_compile_options(property_catch2_test PRIVATE /W4)
else()
    target_compile_options(property_catch2_test PRIVATE -Wall -Wextra -Wpedantic)
endif()
