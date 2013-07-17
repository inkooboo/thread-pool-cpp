#include "thread_pool.hpp"

#include "mpsc_bounded_queue.hpp"

#include <iostream>
#include <chrono>
#include <thread>
#include <cassert>

static const size_t REPOST_COUNT = 1000000;

struct repost_job_t
{
    thread_pool_t *thread_pool;
    size_t counter;
    long long int begin_count;

    explicit repost_job_t(thread_pool_t *thread_pool)
        : thread_pool(thread_pool)
        , counter(0)
    {
        begin_count = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    }

    void operator()()
    {
        if (counter++ < REPOST_COUNT)
        {
            if (thread_pool)
            {
                thread_pool->post(*this);
            }
        }
        else
        {
            long long int end_count = std::chrono::high_resolution_clock::now().time_since_epoch().count();
            std::cout << "reposted " << counter
                      << " in " << (double)(end_count - begin_count)/(double)1000000 << " ms"
                      << std::endl;
        }
    }
};

struct copy_task_t
{
    copy_task_t()
    {
        std::cout << "default constructor" << std::endl;
    }

    copy_task_t(const copy_task_t &)
    {
        std::cout << "copy constructor" << std::endl;
    }

    copy_task_t(copy_task_t &&)
    {
        std::cout << "move constructor" << std::endl;
    }

    copy_task_t & operator==(const copy_task_t &)
    {
        std::cout << "copy operator" << std::endl;
        return *this;
    }

    copy_task_t & operator==(const copy_task_t &&)
    {
        std::cout << "move operator" << std::endl;
        return *this;
    }

    void operator()()
    {
        std::cout << "operator()()" << std::endl;
    }
};

void test_queue()
{
    mpsc_bounded_queue_t<int, 2> queue;
    int e = 0;
    assert(!queue.move_pop(e));
    assert(queue.move_push(1));
    assert(queue.move_push(2));
    assert(!queue.move_push(3));
    assert(queue.move_pop(e) && e == 1);
    assert(queue.move_pop(e) && e == 2);
    assert(!queue.move_pop(e));
    assert(queue.move_push(3));
    assert(queue.move_pop(e) && e == 3);
}

int main(int, const char *[])
{
    test_queue();

    thread_pool_t thread_pool;

    thread_pool.post(copy_task_t());

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    thread_pool.post(repost_job_t(&thread_pool));
    thread_pool.post(repost_job_t(&thread_pool));
    thread_pool.post(repost_job_t(&thread_pool));
    thread_pool.post(repost_job_t(&thread_pool));

    std::cout << "See processor usage and hit ENTER to continue" << std::endl;
    std::cin.get();

    return 0;
}
