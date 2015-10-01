#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP

#include <worker.hpp>
#include <atomic>
#include <stdexcept>
#include <memory>
#include <vector>
#include <future>
#include <type_traits>

/**
 * @brief The ThreadPoolOptions struct provides construction options for ThreadPool.
 */
struct ThreadPoolOptions {
    size_t threads_count{std::thread::hardware_concurrency()};
    size_t worker_queue_size = 1024;
    Worker::OnStart onStart;
    Worker::OnStop onStop;
};

/**
 * @brief The ThreadPool class implements thread pool pattern.
 * It is highly scalable and fast.
 * It is header only.
 * It implements both work-stealing and work-distribution balancing startegies.
 * It implements cooperative scheduling strategy for tasks.
 */
class ThreadPool {
public:
    /**
     * @brief ThreadPool Construct and start new thread pool.
     * @param options Creation options.
     */
    explicit ThreadPool(const ThreadPoolOptions &options = ThreadPoolOptions());

    /**
     * @brief ~ThreadPool Stop all workers and destroy thread pool.
     */
    ~ThreadPool();

    /**
     * @brief post Post piece of job to thread pool.
     * @param handler Handler to be called from thread pool worker. It has to be callable as 'handler()'.
     * @throws std::overflow_error if worker's queue is full.
     * @note All exceptions thrown by handler will be suppressed. Use 'process()' to get result of handler's
     * execution or exception thrown.
     */
    template <typename Handler>
    void post(Handler &&handler);

    /**
     * @brief process Post piece of job to thread pool and get future for this job.
     * @param handler Handler to be called from thread pool worker. It has to be callable as 'handler()'.
     * @return Future which hold handler result or exception thrown.
     * @throws std::overflow_error if worker's queue is full.
     * @note This method of posting job to thread pool is much slower than 'post()' due to std::future and
     * std::packaged_task construction overhead.
     */
    template <typename Handler, typename R = typename std::result_of<Handler()>::type>
    typename std::future<R> process(Handler &&handler);
    
    /**
     * @brief getWorkerCount Returns the number of workers created by the thread pool
     * @return Worker ID.
     */
    size_t getWorkerCount() const;

private:
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool & operator=(const ThreadPool&) = delete;

    Worker & getWorker();

    std::vector<std::unique_ptr<Worker>> m_workers;
    std::atomic<size_t> m_next_worker;
};


/// Implementation

inline ThreadPool::ThreadPool(const ThreadPoolOptions &options)
    : m_next_worker(0)
{
    size_t workers_count = options.threads_count;

    if (0 == workers_count) {
        workers_count = 1;
    }

    m_workers.resize(workers_count);
    for (auto &worker_ptr : m_workers) {
        worker_ptr.reset(new Worker(options.worker_queue_size));
    }

    for (size_t i = 0; i < m_workers.size(); ++i) {
        Worker *steal_donor = m_workers[(i + 1) % m_workers.size()].get();
        m_workers[i]->start(i, steal_donor, options.onStart, options.onStop);
    }
}

inline ThreadPool::~ThreadPool()
{
    for (auto &worker_ptr : m_workers) {
        worker_ptr->stop();
    }
}

template <typename Handler>
inline void ThreadPool::post(Handler &&handler)
{
    if (!getWorker().post(std::forward<Handler>(handler)))
    {
        throw std::overflow_error("worker queue is full");
    }
}

template <typename Handler, typename R>
typename std::future<R> ThreadPool::process(Handler &&handler)
{
    std::packaged_task<R()> task([handler = std::move(handler)] () {
        return handler();
    });

    std::future<R> result = task.get_future();

    if (!getWorker().post(task))
    {
        throw std::overflow_error("worker queue is full");
    }

    return result;
}


inline Worker & ThreadPool::getWorker()
{
    size_t id = Worker::getWorkerIdForCurrentThread();

    if (id > m_workers.size()) {
        id = m_next_worker.fetch_add(1, std::memory_order_relaxed) % m_workers.size();
    }

    return *m_workers[id];
}

inline size_t ThreadPool::getWorkerCount() const {
    return m_workers.size();
}

#endif

