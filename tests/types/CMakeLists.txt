add_executable(type_system_test TypeSystemTest.cpp)
target_include_directories(type_system_test PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(type_system_test PRIVATE Catch2::Catch2WithMain types)
