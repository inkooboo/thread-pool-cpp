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
    Worker<>::OnStart onStart;
    Worker<>::OnStop onStop;
};

/**
 * @brief The ThreadPool class implements thread pool pattern.
 * It is highly scalable and fast.
 * It is header only.
 * It implements both work-stealing and work-distribution balancing startegies.
 * It implements cooperative scheduling strategy for tasks.
 */

template <size_t STORAGE_SIZE = 128>
class ThreadPool {
public:
    
    typedef Worker<STORAGE_SIZE> FixedWorker;
    
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
    void post(Handler &&handler)
    {
        if (!getWorker().post(std::forward<Handler>(handler))) {
            throw std::overflow_error("worker queue is full");
        }
    }
    
    /**
     * @brief process Post piece of job to thread pool and get future for this job.
     * @param handler Handler to be called from thread pool worker. It has to be callable as 'handler()'.
     * @return Future which hold handler result or exception thrown.
     * @throws std::overflow_error if worker's queue is full.
     * @note This method of posting job to thread pool is much slower than 'post()' due to std::future and
     * std::packaged_task construction overhead.
     */
    template <typename Handler, typename R = typename std::result_of<Handler()>::type>
    typename std::future<R> process(Handler &&handler)
    {
        std::packaged_task<R()> task([handler = std::move(handler)] () {
            return handler();
        });
        
        std::future<R> result = task.get_future();
        
        if (!getWorker().post(task)) {
            throw std::overflow_error("worker queue is full");
        }
        
        return result;
    }
    
    /**
     * @brief getWorkerCount Returns the number of workers created by the thread pool
     * @return Worker ID.
     */
    size_t getWorkerCount() const;

private:
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool & operator=(const ThreadPool&) = delete;

    FixedWorker & getWorker();

    std::vector<std::unique_ptr<FixedWorker>> m_workers;
    std::atomic<size_t> m_next_worker;
};


/// Implementation

template <size_t STORAGE_SIZE>
inline ThreadPool<STORAGE_SIZE>::ThreadPool(const ThreadPoolOptions &options)
    : m_next_worker(0)
{
    size_t workers_count = options.threads_count;

    if (0 == workers_count) {
        workers_count = 1;
    }

    m_workers.resize(workers_count);
    for (auto &worker_ptr : m_workers) {
        worker_ptr.reset(new FixedWorker(options.worker_queue_size));
    }

    for (size_t i = 0; i < m_workers.size(); ++i) {
        FixedWorker *steal_donor = m_workers[(i + 1) % m_workers.size()].get();
        m_workers[i]->start(i, steal_donor, options.onStart, options.onStop);
    }
}

template <size_t STORAGE_SIZE>
inline ThreadPool<STORAGE_SIZE>::~ThreadPool()
{
    for (auto &worker_ptr : m_workers) {
        worker_ptr->stop();
    }
}

template <size_t STORAGE_SIZE>
inline typename ThreadPool<STORAGE_SIZE>::FixedWorker& ThreadPool<STORAGE_SIZE>::getWorker()
{
    size_t id = FixedWorker::getWorkerIdForCurrentThread();

    if (id > m_workers.size()) {
        id = m_next_worker.fetch_add(1, std::memory_order_relaxed) % m_workers.size();
    }

    return *m_workers[id];
}


#endif

