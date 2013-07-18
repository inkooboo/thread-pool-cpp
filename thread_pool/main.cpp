#include "thread_pool.hpp"

#include "asio_thread_pool.hpp"

#include "mpsc_bounded_queue.hpp"

#include <iostream>
#include <chrono>
#include <thread>
#include <cassert>

static const size_t REPOST_COUNT = 100000;

struct repost_job_t
{
    typedef std::function<void()> task_t;
    typedef std::function<void(task_t&&)> poster_t;
    poster_t *post_method;
    size_t counter;
    long long int begin_count;

    explicit repost_job_t(poster_t *post_method)
        : post_method(post_method)
        , counter(0)
    {
        begin_count = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    }

    void operator()()
    {
        if (counter++ < REPOST_COUNT)
        {
            if (post_method)
            {
                (*post_method)(*this);
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

void test_standalone_func()
{
}

int main(int, const char *[])
{
    using namespace std::placeholders;

    test_queue();

    {
        thread_pool_t thread_pool(2);

        thread_pool.post(test_standalone_func);
        thread_pool.post(copy_task_t());

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        repost_job_t::poster_t poster = std::bind(&thread_pool_t::post<repost_job_t::task_t>, &thread_pool, _1);

        thread_pool.post(repost_job_t(&poster));
        thread_pool.post(repost_job_t(&poster));
        thread_pool.post(repost_job_t(&poster));
        thread_pool.post(repost_job_t(&poster));

        std::cout << "thread_pool_t" << std::endl;
        std::cout << "See processor usage and hit ENTER to continue" << std::endl;
        std::cin.get();
    }

    {
        asio_thread_pool_t asio_thread_pool(2);

        repost_job_t::poster_t asio_poster = std::bind(&asio_thread_pool_t::post<repost_job_t::task_t>, &asio_thread_pool, _1);

        asio_thread_pool.post(repost_job_t(&asio_poster));
        asio_thread_pool.post(repost_job_t(&asio_poster));
        asio_thread_pool.post(repost_job_t(&asio_poster));
        asio_thread_pool.post(repost_job_t(&asio_poster));

        std::cout << "asio_thread_pool_t" << std::endl;
        std::cout << "See processor usage and hit ENTER to continue" << std::endl;
        std::cin.get();
    }

    return 0;
}
