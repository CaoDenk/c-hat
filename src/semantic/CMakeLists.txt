file(GLOB_RECURSE SEMANTIC_SOURCES "*.cpp")
file(GLOB_RECURSE SEMANTIC_HEADERS "*.h")

add_library(semantic STATIC ${SEMANTIC_SOURCES} ${SEMANTIC_HEADERS})
target_include_directories(semantic PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(semantic PUBLIC ast parser types)
