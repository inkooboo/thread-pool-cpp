#tests

enable_testing()

# Explicit thread linking should support all targets:
# what():  Enable multithreading to use std::thread: Operation not permitted
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Build tests
add_executable(fixed_function_test fixed_function.t.cpp)
target_link_libraries(fixed_function_test Threads::Threads)

add_executable(thread_pool_test thread_pool.t.cpp thread_pool2.t.cpp)
target_link_libraries(thread_pool_test Threads::Threads)

# Run tests for host == target platforms
if(NOT IOS AND NOT ANDROID)
  add_test(NAME FixedFunctionTest COMMAND fixed_function_test)
  add_test(NAME ThreadPoolTest COMMAND thread_pool_test)
endif()
