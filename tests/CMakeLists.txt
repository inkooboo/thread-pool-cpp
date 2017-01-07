#tests

enable_testing()

add_executable(fixed_function_test fixed_function.t.cpp)
target_link_libraries(fixed_function_test)
add_test(NAME FixedFunctionTest COMMAND fixed_function_test)

add_executable(thread_pool_test thread_pool.t.cpp thread_pool2.t.cpp)
target_link_libraries(thread_pool_test)
add_test(NAME ThreadPoolTest COMMAND thread_pool_test)
