cmake_minimum_required (VERSION 3.5)

project (CLIPP_example)
aux_source_directory(. DIR_SRCS)

set(CMAKE_CXX_STANDARD 20)

find_package(fmt REQUIRED)

add_executable(CLIPP_interactive example1.cpp)
add_executable(CLIPP_cli_arg example2.cpp)
target_link_libraries(CLIPP_interactive PRIVATE fmt::fmt CLI++)
target_link_libraries(CLIPP_cli_arg PRIVATE fmt::fmt CLI++)