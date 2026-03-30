find_package(Catch2 3 REQUIRED)
include_directories("D:/vcpkg/installed/x64-windows/include")

add_executable(immutable_method_catch2_test ImmutableMethodTest.cpp)
target_include_directories(immutable_method_catch2_test PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(immutable_method_catch2_test PRIVATE Catch2::Catch2WithMain lexer ast parser semantic types)

## Expose no-main test runner interface (guard against redefinition)
if(NOT TARGET immutable_method_runner_ext)
add_library(immutable_method_runner_ext STATIC
  runner/ITestRunner.cpp
)
target_include_directories(immutable_method_runner_ext PRIVATE
  ${CMAKE_SOURCE_DIR}/tests/immutable_method/runner
)
add_executable(immutable_method_runner_exe
  runner/runner_main.cpp
)
target_link_libraries(immutable_method_runner_exe PRIVATE immutable_method_runner_ext)
endif()
