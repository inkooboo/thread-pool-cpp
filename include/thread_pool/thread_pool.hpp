#pragma once

#include <thread_pool/worker.hpp>

#include <atomic>
#include <memory>
#include <stdexcept>
#include <vector>

namespace tp
{

/**
 * @brief The ThreadPoolOptions struct provides construction options for
 * ThreadPool.
 */
struct ThreadPoolOptions
{
    enum
    {
        AUTODETECT = 0
    };

    size_t threads_count = AUTODETECT;
    size_t worker_queue_size = 1024;
};

/**
 * @brief The ThreadPool class implements thread pool pattern.
 * It is highly scalable and fast.
 * It is header only.
 * It implements both work-stealing and work-distribution balancing
 * startegies.
 * It implements cooperative scheduling strategy for tasks.
 */
template <size_t TASK_SIZE>
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
    Worker<TASK_SIZE>& getWorker();

    std::vector<std::unique_ptr<Worker<TASK_SIZE>>> m_workers;
    std::atomic<size_t> m_next_worker;
};

using ThreadPool = ThreadPoolImpl<128>;


/// Implementation

template <size_t TASK_SIZE>
inline ThreadPoolImpl<TASK_SIZE>::ThreadPoolImpl(
                                            const ThreadPoolOptions& options)
    : m_next_worker(0)
{
    size_t workers_count = options.threads_count;

    if(ThreadPoolOptions::AUTODETECT == options.threads_count)
    {
        workers_count = std::thread::hardware_concurrency();
    }

    if(0 == workers_count)
    {
        workers_count = 1;
    }

    m_workers.resize(workers_count);
    for(auto& worker_ptr : m_workers)
    {
        worker_ptr.reset(new Worker<TASK_SIZE>(options.worker_queue_size));
    }

    for(size_t i = 0; i < m_workers.size(); ++i)
    {
        Worker<TASK_SIZE>* steal_donor =
                                m_workers[(i + 1) % m_workers.size()].get();
        m_workers[i]->start(i, steal_donor);
    }
}

template <size_t TASK_SIZE>
inline ThreadPoolImpl<TASK_SIZE>::ThreadPoolImpl(ThreadPoolImpl<TASK_SIZE>&& rhs) noexcept
{
    *this = rhs;
}

template <size_t TASK_SIZE>
inline ThreadPoolImpl<TASK_SIZE>::~ThreadPoolImpl()
{
    for (auto& worker_ptr : m_workers)
    {
        worker_ptr->stop();
    }
}

template <size_t TASK_SIZE>
inline ThreadPoolImpl<TASK_SIZE>&
ThreadPoolImpl<TASK_SIZE>::operator=(ThreadPoolImpl<TASK_SIZE>&& rhs) noexcept
{
    if (this != &rhs)
    {
        m_workers = std::move(rhs.m_workers);
        m_next_worker = rhs.m_next_worker.load();
    }
    return *this;
}

template <size_t TASK_SIZE>
template <typename Handler>
inline bool ThreadPoolImpl<TASK_SIZE>::tryPost(Handler&& handler)
{
    return getWorker().post(std::forward<Handler>(handler));
}

template <size_t TASK_SIZE>
template <typename Handler>
inline void ThreadPoolImpl<TASK_SIZE>::post(Handler&& handler)
{
    const auto ok = tryPost(std::forward<Handler>(handler));
    if (!ok)
    {
        throw std::runtime_error("thread pool queue is full");
    }
}

template <size_t TASK_SIZE>
inline Worker<TASK_SIZE>& ThreadPoolImpl<TASK_SIZE>::getWorker()
{
    auto id = Worker<TASK_SIZE>::getWorkerIdForCurrentThread();

    if (id > m_workers.size())
    {
        id = m_next_worker.fetch_add(1, std::memory_order_relaxed) %
             m_workers.size();
    }

    return *m_workers[id];
}
}
