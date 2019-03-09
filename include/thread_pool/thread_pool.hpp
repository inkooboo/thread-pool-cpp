#pragma once

#if defined __SUNPRO_CC
#include <functional>
#else
#include <fixed_function.hpp>
#endif
#include <mpmc_bounded_queue.hpp>
#include <thread_pool_options.hpp>
#include <worker.hpp>

#include <stdexcept>
#include <atomic>
#include <memory>
#include <vector>
#include <utility>

#if defined __sun__
#include <cstdio>		/* For fprintf */
#include <sys/types.h>
#include <sys/processor.h>
#include <sys/procset.h>
#include <unistd.h>		/* For sysconf */
#elif defined __linux__
#include <cstdio>		/* For fprintf */
#include <sched.h>
#elif defined __FreeBSD__
#include <cstdio>		/* For fprintf */
#include <pthread_np.h>
#endif

namespace tp
{

#if defined __sun__ || defined __linux__ || defined __FreeBSD__
static bool v_affinity = false;	/* Default: disabled */
#endif

template <typename Task, template<typename> class Queue>
class ThreadPoolImpl;

#if defined __SUNPRO_CC
using ThreadPool = ThreadPoolImpl<std::function<void()>,
                                  MPMCBoundedQueue>;
#else
using ThreadPool = ThreadPoolImpl<FixedFunction<void(), 128>,
                                  MPMCBoundedQueue>;
#endif

/**
 * @brief The ThreadPool class implements thread pool pattern.
 * It is highly scalable and fast.
 * It is header only.
 * It implements both work-stealing and work-distribution balancing
 * startegies.
 * It implements cooperative scheduling strategy for tasks.
 */
template <typename Task, template<typename> class Queue>
class ThreadPoolImpl {

using WorkerVector = std::vector<std::unique_ptr<Worker<Task, Queue>>>;

public:
    /**
     * @brief ThreadPool Construct and start new thread pool.
     * @param options Creation options.
     */
    explicit ThreadPoolImpl(
        const ThreadPoolOptions& options = ThreadPoolOptions());

    /**
     * @brief Move ctor implementation.
     */
    ThreadPoolImpl(ThreadPoolImpl&& rhs) noexcept;

    /**
     * @brief ~ThreadPool Stop all workers and destroy thread pool.
     */
    ~ThreadPoolImpl();

    /**
     * @brief Move assignment implementaion.
     */
    ThreadPoolImpl& operator=(ThreadPoolImpl&& rhs) noexcept;

    /**
     * @brief post Try post job to thread pool.
     * @param handler Handler to be called from thread pool worker. It has
     * to be callable as 'handler()'.
     * @return 'true' on success, false otherwise.
     * @note All exceptions thrown by handler will be suppressed.
     */
    template <typename Handler>
    bool tryPost(Handler&& handler);

    /**
     * @brief post Post job to thread pool.
     * @param handler Handler to be called from thread pool worker. It has
     * to be callable as 'handler()'.
     * @throw std::overflow_error if worker's queue is full.
     * @note All exceptions thrown by handler will be suppressed.
     */
    template <typename Handler>
    void post(Handler&& handler);
private:
    Worker<Task, Queue>& getWorker();

    WorkerVector m_workers;
    std::atomic<std::size_t> m_next_worker;

    #if defined __sun__ || defined __linux__ || defined __FreeBSD__
    std::size_t v_cpu = 0, v_cpu_max = std::thread::hardware_concurrency() - 1;
    #endif

    #if defined __sun__
    std::vector<processorid_t> v_cpu_id;	/* Struct for CPU/core ID */
    #endif
};

/// Implementation

template <typename Task, template<typename> class Queue>
inline ThreadPoolImpl<Task, Queue>::ThreadPoolImpl(
                                            const ThreadPoolOptions& options)
    : m_workers(options.threadCount())
    , m_next_worker(0)
{
    for(auto& worker_ptr : m_workers)
    {
        worker_ptr.reset(new Worker<Task, Queue>(options.queueSize()));
    }

    #if defined __sun__
    if (v_affinity) {
        for (processorid_t i = 0; i <= sysconf(_SC_CPUID_MAX); ++i) {
            if (p_online(i, P_STATUS) == P_ONLINE)	/* Get only online cores ID */
                v_cpu_id.push_back(i);
        }
    }
    #endif

    for(std::size_t i = 0; i < m_workers.size(); ++i)
    {
	#if defined __sun__ || defined __linux__ || defined __FreeBSD__
        if (v_affinity) {
            if (v_cpu > v_cpu_max)
                v_cpu = 0;
            #if defined __linux__
            cpu_set_t mask;
            #elif defined __FreeBSD__
            cpuset_t mask;
            #endif
            #if defined __linux__ || defined __FreeBSD__
            CPU_ZERO(&mask);
            CPU_SET(v_cpu, &mask);
            pthread_t v_thread = pthread_self();
            #endif
            #if defined __linux__
            if (pthread_setaffinity_np(v_thread, sizeof(cpu_set_t), &mask) != 0)
            #elif defined __FreeBSD__
            if (pthread_setaffinity_np(v_thread, sizeof(cpuset_t), &mask) != 0)
            #elif defined __sun__
            if (processor_bind(P_LWPID, P_MYID, v_cpu_id[v_cpu], NULL) != 0)
            #endif
                fprintf(stderr, "Error setting thread affinity\n");
            ++v_cpu;
        }
	#endif

        m_workers[i]->start(i, m_workers);
    }
}

template <typename Task, template<typename> class Queue>
inline ThreadPoolImpl<Task, Queue>::ThreadPoolImpl(ThreadPoolImpl<Task, Queue>&& rhs) noexcept
{
    *this = rhs;
}

template <typename Task, template<typename> class Queue>
inline ThreadPoolImpl<Task, Queue>::~ThreadPoolImpl()
{
    for (auto& worker_ptr : m_workers)
    {
        worker_ptr->stop();
    }
}

template <typename Task, template<typename> class Queue>
inline ThreadPoolImpl<Task, Queue>&
ThreadPoolImpl<Task, Queue>::operator=(ThreadPoolImpl<Task, Queue>&& rhs) noexcept
{
    if (this != &rhs)
    {
        m_workers = std::move(rhs.m_workers);
        m_next_worker = rhs.m_next_worker.load();
    }
    return *this;
}

template <typename Task, template<typename> class Queue>
template <typename Handler>
inline bool ThreadPoolImpl<Task, Queue>::tryPost(Handler&& handler)
{
    return getWorker().tryPost(std::forward<Handler>(handler));
}

template <typename Task, template<typename> class Queue>
template <typename Handler>
inline void ThreadPoolImpl<Task, Queue>::post(Handler&& handler)
{
    const auto ok = tryPost(std::forward<Handler>(handler));
    if (!ok)
    {
        throw std::runtime_error("thread pool queue is full");
    }
}

template <typename Task, template<typename> class Queue>
inline Worker<Task, Queue>& ThreadPoolImpl<Task, Queue>::getWorker()
{
    auto id = Worker<Task, Queue>::getWorkerIdForCurrentThread();

    if (id > m_workers.size())
    {
        id = m_next_worker.fetch_add(1, std::memory_order_relaxed) %
             m_workers.size();
    }

    return *m_workers[id];
}

}
