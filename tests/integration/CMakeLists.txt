add_executable(integration_catch2_test IntegrationTest.cpp)
target_include_directories(integration_catch2_test PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(integration_catch2_test PRIVATE Catch2::Catch2WithMain
  lexer ast parser semantic types llvm_codegen)

if(MSVC)
  target_compile_options(integration_catch2_test PRIVATE /FS)
  # 禁用 PDB 生成以避免 LLVM 调试库的 PDB 冲突
  target_link_options(integration_catch2_test PRIVATE
    $<$<CONFIG:Debug>:/DEBUG:NONE>)
endif()
