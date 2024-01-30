cmake_minimum_required (VERSION 3.5)

project (CLIPP_test)
aux_source_directory(. DIR_SRCS)

set(CMAKE_CXX_STANDARD 20)

find_package(fmt REQUIRED)

add_executable(CLIPP_test ${DIR_SRCS})
target_link_libraries(CLIPP_test PRIVATE fmt::fmt CLI++)