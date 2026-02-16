file(GLOB_RECURSE AST_SOURCES "*.cpp")
file(GLOB_RECURSE AST_HEADERS "*.h")

add_library(ast STATIC ${AST_SOURCES} ${AST_HEADERS})
target_include_directories(ast PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
