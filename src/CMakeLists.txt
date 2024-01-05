cmake_minimum_required (VERSION 3.5)

project (CLIPP_lib)
aux_source_directory(. DIR_SRCS)

set(CMAKE_CXX_STANDARD 20)

find_package(fmt)

add_library(CLI++ STATIC ${DIR_SRCS})
target_link_libraries(CLI++ fmt::fmt)
set_target_properties(CLI++ PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../lib)