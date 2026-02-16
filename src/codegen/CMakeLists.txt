file(GLOB_RECURSE CODEGEN_SOURCES "*.cpp")
file(GLOB_RECURSE CODEGEN_HEADERS "*.h")

add_library(code_generator STATIC ${CODEGEN_SOURCES} ${CODEGEN_HEADERS})
target_include_directories(code_generator PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(code_generator PUBLIC ast semantic types)
