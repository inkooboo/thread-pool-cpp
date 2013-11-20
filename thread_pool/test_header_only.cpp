#include <thread_pool.hpp>

void test_header_only()
{
    thread_pool_t thread_pool;
    thread_pool.post([](){});
}
