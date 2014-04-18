#define WITHOUT_ASIO 1

#include <thread_pool.hpp>

#ifndef WITHOUT_ASIO
#include <asio_thread_pool.hpp>
#endif

#include <iostream>
#include <chrono>
#include <thread>
#include <cassert>
#include <vector>
#include <future>

static const size_t THREADS_COUNT = 2;
static const size_t CONCURRENCY = 4;
static const size_t REPOST_COUNT = 1000000;

struct Heavy {
    bool verbose;
    std::vector<char> resource;

    Heavy(bool verbose = false)
        : verbose(verbose)
        , resource(100*1024*1024)
    {
        if (verbose) {
            std::cout << "heavy default constructor" << std::endl;
        }
    }

    Heavy(const Heavy &o)
        : verbose(o.verbose)
        , resource(o.resource)
    {
        if (verbose) {
            std::cout << "heavy copy constructor" << std::endl;
        }
    }

    Heavy(Heavy &&o)
        : verbose(o.verbose)
        , resource(std::move(o.resource))
    {
        if (verbose) {
            std::cout << "heavy move constructor" << std::endl;
        }
    }

    Heavy & operator==(const Heavy &o)
    {
        verbose = o.verbose;
        resource = o.resource;
        if (verbose) {
            std::cout << "heavy copy operator" << std::endl;
        }
        return *this;
    }

    Heavy & operator==(const Heavy &&o)
    {
        verbose = o.verbose;
        resource = std::move(o.resource);
        if (verbose) {
            std::cout << "heavy move operator" << std::endl;
        }
        return *this;
    }

    ~Heavy()
    {
        if (verbose) {
            std::cout << "heavy destructor. " << (resource.size() ? "Own resource" : "Don't own resource") << std::endl;
        }
    }
};


struct RepostJob {
    //Heavy heavy;

    ThreadPool *thread_pool;
#ifndef WITHOUT_ASIO
    AsioThreadPool *asio_thread_pool;
#endif

    size_t counter;
    long long int begin_count;

    RepostJob()
        : thread_pool(0)
#ifndef WITHOUT_ASIO
        , asio_thread_pool(0)
#endif
        , counter(0)
    {
        begin_count = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    }


    explicit RepostJob(ThreadPool *thread_pool)
        : thread_pool(thread_pool)
#ifndef WITHOUT_ASIO
            , asio_thread_pool(0)
#endif
        , counter(0)
    {
        begin_count = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    }

#ifndef WITHOUT_ASIO
    explicit repost_job_t(asio_thread_pool_t *asio_thread_pool)
        : thread_pool(0)
        , asio_thread_pool(asio_thread_pool)
        , counter(0)
    {
        begin_count = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    }
#endif

    void operator()()
    {
        if (counter++ < REPOST_COUNT) {
#ifndef WITHOUT_ASIO
            if (asio_thread_pool) {
                asio_thread_pool->post(*this);
                return;
            }
#endif
            if (thread_pool) {
                thread_pool->post(*this);
                return;
            }
        }
        else {
            long long int end_count = std::chrono::high_resolution_clock::now().time_since_epoch().count();
            std::cout << "reposted " << counter
                      << " in " << (double)(end_count - begin_count)/(double)1000000 << " ms"
                      << std::endl;
        }
    }
};

struct CopyTask
{
    Heavy heavy;

    CopyTask() : heavy(true) {}

    void operator()()
    {
        std::cout << "copy_task_t::operator()()" << std::endl;
    }
};

void test_standalone_func()
{
}

struct TestMember
{
    void useless(int i, int j)
    {
        (void)(i + j);
    }

} test_member;

int main(int, const char *[])
{
    using namespace std::placeholders;

    std::cout << "*******begin tests*******" << std::endl;
    {
        std::cout << "***thread_pool_t***" << std::endl;

        {
            ThreadPoolOptions options;
            options.threads_count = THREADS_COUNT;
            ThreadPool thread_pool(options);
            thread_pool.post(test_standalone_func);
            thread_pool.post(std::bind(&TestMember::useless, &test_member, 42, 42));
        }

        {
            std::cout << "Copy test [ENTER]" << std::endl;
            ThreadPoolOptions options;
            options.threads_count = THREADS_COUNT;
            ThreadPool thread_pool(options);
            thread_pool.post(CopyTask());
        }

        {
            ThreadPoolOptions options;
            options.threads_count = THREADS_COUNT;
            ThreadPool thread_pool(options);
            for (size_t i = 0; i < CONCURRENCY; ++i) {
                thread_pool.post(RepostJob(&thread_pool));
            }

            std::cout << "Repost test [ENTER]" << std::endl;
            std::cin.get();
        }
    }

#ifndef WITHOUT_ASIO
    {
        std::cout << "***asio_thread_pool_t***" << std::endl;

        AsioThreadPool asio_thread_pool(THREADS_COUNT);

        std::cout << "Copy test [ENTER]" << std::endl;
        asio_thread_pool.post(copy_task_t());

        for (size_t i = 0; i < CONCURRENCY; ++i) {
            asio_thread_pool.post(repost_job_t(&asio_thread_pool));
        }

        std::cout << "Repost test [ENTER]" << std::endl;
        std::cin.get();
    }
#endif

    return 0;
}
