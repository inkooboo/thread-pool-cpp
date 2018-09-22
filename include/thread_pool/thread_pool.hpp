#pragma once

#include <fixed_function.hpp>
#include <mpmc_bounded_queue.hpp>
#include <thread_pool_options.hpp>
#include <worker.hpp>

#include <atomic>
#include <memory>
#include <stdexcept>
#include <vector>

#ifdef AFFINITY
#ifdef __sun__
#include <sys/types.h>
#include <sys/processor.h>
#include <sys/procset.h>
#include <unistd.h>		/* For sysconf */
#elif __linux__
#include <cstdio>		/* For fprintf */
#include <sched.h>
#endif
#endif

namespace tp
{

template <typename Task, template<typename> class Queue>
class ThreadPoolImpl;
using ThreadPool = ThreadPoolImpl<FixedFunction<void(), 128>,
                                  MPMCBoundedQueue>;

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

    std::vector<std::unique_ptr<Worker<Task, Queue>>> m_workers;
    std::atomic<std::size_t> m_next_worker;

    #if (defined __sun__ || defined __linux__) && defined AFFINITY
    std::size_t v_cpu = 0;
    std::size_t v_cpu_max = std::thread::hardware_concurrency() - 1;
    #endif

    #if defined __sun__  && defined AFFINITY
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

    #if defined __sun__  && defined AFFINITY
    processorid_t i, cpuid_max;
    cpuid_max = sysconf(_SC_CPUID_MAX);
    for (i = 0; i <= cpuid_max; ++i) {
        if (p_online(i, P_STATUS) != -1)	/* Get only online cores ID */
            v_cpu_id.push_back(i);
    }
    #endif

    for(std::size_t i = 0; i < m_workers.size(); ++i)
    {
        Worker<Task, Queue>* steal_donor =
                                m_workers[(i + 1) % m_workers.size()].get();

	#if (defined __sun__ || defined __linux__) && defined AFFINITY
	if (v_cpu > v_cpu_max) {
		v_cpu = 0;
	}

	#ifdef __sun__
	processor_bind(P_LWPID, P_MYID, v_cpu_id[v_cpu], NULL);
	#elif __linux__
	cpu_set_t mask;
	CPU_ZERO(&mask);
	CPU_SET(v_cpu, &mask);
	pthread_t thread = pthread_self();
	if (pthread_setaffinity_np(thread, sizeof(cpu_set_t), &mask) != 0) {
		fprintf(stderr, "Error setting thread affinity\n");
	}
	#endif

	++v_cpu;
	#endif

        m_workers[i]->start(i, steal_donor);
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
    return getWorker().post(std::forward<Handler>(handler));
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
