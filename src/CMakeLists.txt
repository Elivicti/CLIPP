cmake_minimum_required (VERSION 3.5)

project (CLIPP_lib)
aux_source_directory(. DIR_SRCS)

set(CMAKE_CXX_STANDARD 20)

find_package(fmt REQUIRED)
find_library(READLINE_LIBRARY readline REQUIRED)

add_library(CLI++ STATIC ${DIR_SRCS})
target_link_libraries(CLI++ PRIVATE fmt::fmt readline)
set_target_properties(CLI++ PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../lib)