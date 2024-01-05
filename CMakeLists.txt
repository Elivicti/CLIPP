cmake_minimum_required (VERSION 3.5)

project (CLIPP)

set(CMAKE_CXX_STANDARD 20)
# set(ARCHIVE_OUTPUT_DIRECTORY  ${PROJECT_BINARY_DIR}/library)

add_subdirectory(src)
add_subdirectory(test)