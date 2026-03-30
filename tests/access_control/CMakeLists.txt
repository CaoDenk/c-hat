# 访问控制测试
add_executable(access_control_test AccessControlTest.cpp)

# 添加包含目录
target_include_directories(access_control_test PRIVATE ${CMAKE_SOURCE_DIR}/src)

# 链接库
target_link_libraries(access_control_test PRIVATE
    lexer
    parser
    ast
    semantic
    types
    Catch2::Catch2WithMain
)

# 添加到测试套件
add_test(NAME access_control_test COMMAND access_control_test)
