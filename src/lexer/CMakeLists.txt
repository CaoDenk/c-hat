file(GLOB_RECURSE LEXER_SOURCES "*.cpp")
file(GLOB_RECURSE LEXER_HEADERS "*.h")

add_library(lexer STATIC ${LEXER_SOURCES} ${LEXER_HEADERS})
target_include_directories(lexer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
