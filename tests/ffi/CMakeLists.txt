find_package(Catch2 3 REQUIRED)

add_executable(ffi_catch2_test FfiTest.cpp)
target_include_directories(ffi_catch2_test PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(ffi_catch2_test PRIVATE Catch2::Catch2WithMain lexer ast parser semantic types)
