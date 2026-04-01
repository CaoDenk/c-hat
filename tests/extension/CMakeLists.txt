add_executable(extension_catch2_test ExtensionTest.cpp)
target_include_directories(extension_catch2_test PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(extension_catch2_test PRIVATE Catch2::Catch2WithMain parser semantic types ast)
