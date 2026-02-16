file(GLOB_RECURSE PARSER_SOURCES "*.cpp")
file(GLOB_RECURSE PARSER_HEADERS "*.h")

add_library(parser STATIC ${PARSER_SOURCES} ${PARSER_HEADERS})
target_include_directories(parser PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(parser PUBLIC ast lexer)
