add_executable(builtin_vars_catch2_test BuiltinVarsTest.cpp)

# 包含头文件目录
target_include_directories(builtin_vars_catch2_test PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)

# 链接库
target_link_libraries(builtin_vars_catch2_test PRIVATE
    parser
    semantic
    Catch2::Catch2WithMain
)

# 添加到测试
add_test(NAME builtin_vars_catch2_test COMMAND builtin_vars_catch2_test)
