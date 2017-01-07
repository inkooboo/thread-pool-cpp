#include <thread_pool/thread_pool.hpp>

#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <future>

using namespace tp;

using ThreadPoolStd = ThreadPool<>;

static const std::size_t CONCURRENCY = 16;
static const std::size_t REPOST_COUNT = 1000000;

struct Heavy
{
    bool verbose;
    std::vector<char> resource;

    Heavy(bool verbose = false) : verbose(verbose), resource(100 * 1024 * 1024)
    {
        if(verbose)
        {
            std::cout << "heavy default constructor" << std::endl;
        }
    }

    Heavy(const Heavy& o) : verbose(o.verbose), resource(o.resource)
    {
        if(verbose)
        {
            std::cout << "heavy copy constructor" << std::endl;
        }
    }

    Heavy(Heavy&& o) : verbose(o.verbose), resource(std::move(o.resource))
    {
        if(verbose)
        {
            std::cout << "heavy move constructor" << std::endl;
        }
    }

    Heavy& operator==(const Heavy& o)
    {
        verbose = o.verbose;
        resource = o.resource;
        if(verbose)
        {
            std::cout << "heavy copy operator" << std::endl;
        }
        return *this;
    }

    Heavy& operator==(const Heavy&& o)
    {
        verbose = o.verbose;
        resource = std::move(o.resource);
        if(verbose)
        {
            std::cout << "heavy move operator" << std::endl;
        }
        return *this;
    }

    ~Heavy()
    {
        if(verbose)
        {
            std::cout << "heavy destructor. "
                      << (resource.size() ? "Owns resource"
                                          : "Doesn't own resource")
                      << std::endl;
        }
    }
};


struct RepostJob
{
    // Heavy heavy;

    ThreadPoolStd* thread_pool;

    volatile std::size_t counter;
    long long int begin_count;
    std::promise<void>* waiter;

    RepostJob(ThreadPoolStd* thread_pool, std::promise<void>* waiter)
        : thread_pool(thread_pool)
        , counter(0)
        , waiter(waiter)
    {
        begin_count = std::chrono::high_resolution_clock::now()
                          .time_since_epoch()
                          .count();
    }

    void operator()()
    {
        if(counter++ < REPOST_COUNT)
        {
            if(thread_pool)
            {
                thread_pool->post(*this);
                return;
            }
        }
        else
        {
            long long int end_count = std::chrono::high_resolution_clock::now()
                                          .time_since_epoch()
                                          .count();
            std::cout << "reposted " << counter << " in "
                      << (double)(end_count - begin_count) / (double)1000000
                      << " ms" << std::endl;
            waiter->set_value();
        }
    }
};

int main(int, const char* [])
{
    std::cout << "Benchmark job reposting" << std::endl;

    {
        std::cout << "***thread pool cpp***" << std::endl;

        std::promise<void> waiters[CONCURRENCY];
        ThreadPoolStd thread_pool;
        for(auto& waiter : waiters)
        {
            thread_pool.post(RepostJob(&thread_pool, &waiter));
        }

        for(auto& waiter : waiters)
        {
            waiter.get_future().wait();
        }
    }

    return 0;
}
