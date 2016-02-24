# thread-poll-cpp build script

project(thread-pool-cpp)

cmake_minimum_required(VERSION 2.8)

ADD_DEFINITIONS(
    -std=c++1y -Wall -Werror -O3
)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")

add_subdirectory(tests)
add_subdirectory(benchmark)

# Install as header-only library
set(INC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include/")
file(GLOB_RECURSE INSTALL_FILES_LIST ${INC_DIR})
set_source_files_properties(${file_list} PROPERTIES HEADER_FILE_ONLY 1)
add_library(HEADER_ONLY_TARGET STATIC ${file_list})
set_target_properties(HEADER_ONLY_TARGET PROPERTIES LINKER_LANGUAGE CXX)
install(DIRECTORY ${INC_DIR} DESTINATION "include")