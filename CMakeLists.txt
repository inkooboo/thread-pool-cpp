# thread-poll-cpp build script

project(thread-pool-cpp)

cmake_minimum_required(VERSION 2.8)

ADD_DEFINITIONS(
    -std=c++1y -Wall -Werror -O3
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/thread_pool)

add_subdirectory(tests)
add_subdirectory(benchmark)
