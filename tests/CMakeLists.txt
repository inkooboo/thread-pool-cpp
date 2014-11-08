project(tests)

cmake_minimum_required(VERSION 2.8)

ADD_DEFINITIONS(
    -std=c++11 -Wall -Werror -O3
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../thread_pool)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_executable(fixed_function fixed_function.t.cpp)
target_link_libraries(fixed_function)

