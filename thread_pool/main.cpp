#include <thread_pool.hpp>

#include <asio_thread_pool.hpp>

#include <mpsc_bounded_queue.hpp>

#include <iostream>
#include <chrono>
#include <thread>
#include <cassert>
#include <vector>
#include <future>

static const size_t THREADS_COUNT = 2;
static const size_t CONCURRENCY = 4;
static const size_t REPOST_COUNT = 100000;

struct heavy_t
{
    bool verbose;
    std::vector<char> resource;

    heavy_t(bool verbose = false)
        : verbose(verbose)
        , resource(100*1024*1024)
    {
        if (!verbose)
            return;
        std::cout << "heavy default constructor" << std::endl;
    }

    heavy_t(const heavy_t &o)
        : verbose(o.verbose)
        , resource(o.resource)
    {
        if (!verbose)
            return;
        std::cout << "heavy copy constructor" << std::endl;
    }

    heavy_t(heavy_t &&o)
        : verbose(o.verbose)
        , resource(std::move(o.resource))
    {
        if (!verbose)
            return;
        std::cout << "heavy move constructor" << std::endl;
    }

    heavy_t & operator==(const heavy_t &o)
    {
        verbose = o.verbose;
        resource = o.resource;
        if (!verbose)
            return *this;
        std::cout << "heavy copy operator" << std::endl;
        return *this;
    }

    heavy_t & operator==(const heavy_t &&o)
    {
        verbose = o.verbose;
        resource = std::move(o.resource);
        if (!verbose)
            return *this;
        std::cout << "heavy move operator" << std::endl;
        return *this;
    }

//    ~heavy_t()
//    {
//        if (!verbose)
//            return;
//        std::cout << "heavy destructor" << std::endl;
//    }
};


struct repost_job_t
{
    //heavy_t heavy;

    thread_pool_t *thread_pool;
    asio_thread_pool_t *asio_thread_pool;

    size_t counter;
    long long int begin_count;

    repost_job_t()
        : thread_pool(0)
        , asio_thread_pool(0)
        , counter(0)
    {
        begin_count = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    }


    explicit repost_job_t(thread_pool_t *thread_pool)
        : thread_pool(thread_pool)
        , asio_thread_pool(0)
        , counter(0)
    {
        begin_count = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    }

    explicit repost_job_t(asio_thread_pool_t *asio_thread_pool)
        : thread_pool(0)
        , asio_thread_pool(asio_thread_pool)
        , counter(0)
    {
        begin_count = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    }

    void operator()()
    {
        if (counter++ < REPOST_COUNT)
        {
            if (asio_thread_pool)
            {
                asio_thread_pool->post(*this);
                return;
            }

            if (thread_pool)
            {
                thread_pool->post(*this);
                return;
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
    heavy_t heavy;

    copy_task_t() : heavy(true) {}

    void operator()()
    {
        std::cout << "copy_task_t::operator()()" << std::endl;
    }
};

void test_queue()
{
    mpsc_bounded_queue_t<int, 2> queue;
    assert(!queue.front());
    assert(queue.push(1));
    assert(queue.push(2));
    assert(!queue.push(3));
    assert(1 == *queue.front());
    queue.pop();
    assert(2 == *queue.front());
    queue.pop();
    assert(!queue.front());
    assert(queue.push(3));
    assert(3 == *queue.front());
    queue.pop();
}

void test_standalone_func()
{
}

struct test_member_t
{
    int useless(int i, int j)
    {
        return i + j;
    }

} test_member;

int main(int, const char *[])
{
    using namespace std::placeholders;

    test_queue();

    std::cout << "*******begin tests*******" << std::endl;
    {
        std::cout << "***thread_pool_t***" << std::endl;

        thread_pool_t thread_pool(THREADS_COUNT);

        thread_pool.post(std::bind(test_standalone_func));
        thread_pool.post(std::bind(&test_member_t::useless, &test_member, 42, 42));

//        std::cout << "Copy test [ENTER]" << std::endl;
//        thread_pool.post(copy_task_t());
//        std::cin.get();

        for (size_t i = 0; i < CONCURRENCY; ++i)
        {
            thread_pool.post(repost_job_t(&thread_pool));
        }

        std::cout << "Repost test [ENTER]" << std::endl;
        std::cin.get();
    }

    {
        std::cout << "***asio_thread_pool_t***" << std::endl;

        asio_thread_pool_t asio_thread_pool(THREADS_COUNT);

//        std::cout << "Copy test [ENTER]" << std::endl;
//        asio_thread_pool.post(copy_task_t());
//        std::cin.get();

        for (size_t i = 0; i < CONCURRENCY; ++i)
        {
            asio_thread_pool.post(repost_job_t(&asio_thread_pool));
        }

        std::cout << "Repost test [ENTER]" << std::endl;
        std::cin.get();
    }

    return 0;
}
