file(GLOB_RECURSE TYPES_SOURCES "*.cpp")
file(GLOB_RECURSE TYPES_HEADERS "*.h")

add_library(types STATIC ${TYPES_SOURCES} ${TYPES_HEADERS})
target_include_directories(types PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
