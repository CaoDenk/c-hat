add_executable(attribute_catch2_test AttributeTest.cpp)
target_include_directories(attribute_catch2_test PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(attribute_catch2_test PRIVATE parser semantic lexer ast types Catch2::Catch2WithMain)
