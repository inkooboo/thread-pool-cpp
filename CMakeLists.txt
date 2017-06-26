cmake_minimum_required(VERSION 2.8)

project(thread-pool-cpp CXX)

option(TESTS "Enable testing" ON)
option(INSTALL "Enable install option" OFF)
option(COVERAGE "Enable code coverage" OFF)
option(BENCHMARK "Build benchmark" OFF)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra -Wsign-compare")
if(COVERAGE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g --coverage")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
endif()


include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)


# gtest
if(TESTS)
  # Tests
  enable_testing()
  add_subdirectory(googletest)
  include_directories(${CMAKE_CURRENT_SOURCE_DIR}/googletest/googletest/include)
  add_subdirectory(tests)
endif()



# Benchmark
if(BENCHMARK)
  add_subdirectory(benchmark)
endif()

if(INSTALL)
  # Install
  file(GLOB_RECURSE INSTALL_FILES_LIST "${CMAKE_CURRENT_SOURCE_DIR}/include/*")
  set_source_files_properties(${INSTALL_FILES_LIST} PROPERTIES HEADER_FILE_ONLY 1)
  add_library(HEADER_ONLY_TARGET STATIC ${INSTALL_FILES_LIST})
  set_target_properties(HEADER_ONLY_TARGET PROPERTIES LINKER_LANGUAGE CXX)
  install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/" DESTINATION "include")
endif()
