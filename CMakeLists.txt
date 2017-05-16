cmake_minimum_required(VERSION 2.8)

project(thread-pool-cpp CXX)

list(APPEND CMAKE_MODULE_PATH
       "${CMAKE_CURRENT_SOURCE_DIR}/cmake"
       "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")

add_definitions(-std=c++14 -Wall -Werror)
if(COVERAGE)
    add_definitions(--coverage)
    set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} --coverage")
endif()

# Get all include files
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_subdirectory(tests)
add_subdirectory(benchmark)

# Install as header-only library
file(GLOB_RECURSE INSTALL_FILES_LIST "${CMAKE_CURRENT_SOURCE_DIR}/include/*")
set_source_files_properties(${INSTALL_FILES_LIST} PROPERTIES HEADER_FILE_ONLY 1)
add_library(HEADER_ONLY_TARGET STATIC ${INSTALL_FILES_LIST})
set_target_properties(HEADER_ONLY_TARGET PROPERTIES LINKER_LANGUAGE CXX)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/" DESTINATION "include")
