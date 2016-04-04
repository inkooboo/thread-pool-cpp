#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP

#include <thread_pool/worker.hpp>
#include <tuple>
#include <atomic>
#include <stdexcept>
#include <memory>
#include <vector>
#include <type_traits>

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

    template <template <typename...> class TPackagedTask>
    struct PackagedTaskAdaptorStd
    {
        template <typename R>
        using type = TPackagedTask<R()>;
    };

    template <template <typename...> class TPackagedTask>
    struct PackagedTaskAdaptorBoost
    {
        template <typename R>
        using type = TPackagedTask<R>;
    };

    template <                        // .
        typename TPackagedTaskAdaptor // .
        >
    struct ThreadPoolSettings
    {
        template <typename R>
        using packaged_task_type =
            typename TPackagedTaskAdaptor::template type<R>;
    };

    /**
     * @brief The ThreadPool class implements thread pool pattern.
     * It is highly scalable and fast.
     * It is header only.
     * It implements both work-stealing and work-distribution balancing
     * startegies.
     * It implements cooperative scheduling strategy for tasks.
     */
    template <typename TSettings>
    class ThreadPool
    {
    public:
        /**
         * @brief ThreadPool Construct and start new thread pool.
         * @param options Creation options.
         */
        explicit ThreadPool(
            const ThreadPoolOptions& options = ThreadPoolOptions());

        /**
         * @brief ~ThreadPool Stop all workers and destroy thread pool.
         */
        ~ThreadPool();

        // TODO:
        template <typename Handler>
        bool try_post(Handler&& handler);

        /**
         * @brief post Post piece of job to thread pool.
         * @param handler Handler to be called from thread pool worker. It has
         * to be
         * callable as 'handler()'.
         * @throws std::overflow_error if worker's queue is full.
         * @note All exceptions thrown by handler will be suppressed. Use
         * 'process()' to get result of handler's
         * execution or exception thrown.
         */
        template <typename Handler>
        void post(Handler&& handler);

        // TODO:
        template <typename Handler,
            typename R = typename std::result_of<Handler()>::type>
        auto try_process(Handler&& handler);


        /**
         * @brief process Post piece of job to thread pool and get future for
         * this
         * job.
         * @param handler Handler to be called from thread pool worker. It has
         * to be
         * callable as 'handler()'.
         * @return Future which hold handler result or exception thrown.
         * @throws std::overflow_error if worker's queue is full.
         * @note This method of posting job to thread pool is much slower than
         * 'post()' due to std::future and
         * std::packaged_task construction overhead.
         */
        template <typename Handler,
            typename R = typename std::result_of<Handler()>::type>
        auto process(Handler&& handler);

    private:
        ThreadPool(const ThreadPool&) = delete;
        ThreadPool& operator=(const ThreadPool&) = delete;

    public:
        ThreadPool(ThreadPool&& rhs)
            : m_workers(std::move(rhs.m_workers)),
              m_next_worker(rhs.m_next_worker.load())
        {
        }

        ThreadPool& operator=(ThreadPool&& rhs)
        {
            m_workers = std::move(rhs.m_workers);
            m_next_worker = rhs.m_next_worker.load();

            return *this;
        }

    private:
        Worker& getWorker();

        std::vector<std::unique_ptr<Worker>> m_workers;
        std::atomic<size_t> m_next_worker;
    };


    /// Implementation

    template <typename TSettings>
    inline ThreadPool<TSettings>::ThreadPool(const ThreadPoolOptions& options)
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
            worker_ptr.reset(new Worker(options.worker_queue_size));
        }

        for(size_t i = 0; i < m_workers.size(); ++i)
        {
            Worker* steal_donor = m_workers[(i + 1) % m_workers.size()].get();
            m_workers[i]->start(i, steal_donor);
        }
    }

    template <typename TSettings>
    inline ThreadPool<TSettings>::~ThreadPool()
    {
        for(auto& worker_ptr : m_workers)
        {
            worker_ptr->stop();
        }
    }

    template <typename TSettings>
    template <typename Handler>
    inline bool ThreadPool<TSettings>::try_post(Handler&& handler)
    {
        return getWorker().post(std::forward<Handler>(handler));
    }

    template <typename TSettings>
    template <typename Handler>
    inline void ThreadPool<TSettings>::post(Handler&& handler)
    {
        if(!try_post(std::forward<Handler>(handler)))
        {
            throw std::overflow_error("worker queue is full");
        }
    }

    template <typename TSettings>
    template <typename Handler, typename R>
    auto ThreadPool<TSettings>::try_process(Handler&& handler)
    {
        using packaged_task_type =
            typename TSettings::template packaged_task_type<R>;

        packaged_task_type task([handler = std::move(handler)]
            {
                return handler();
            });

        auto result = task.get_future();
        return std::make_tuple(try_post(std::move(task)), std::move(result));
    }

    template <typename TSettings>
    template <typename Handler, typename R>
    auto ThreadPool<TSettings>::process(Handler&& handler)
    {
        auto res = try_process(std::forward<Handler>(handler));

        if(std::get<0>(res) == false)
        {
            throw std::overflow_error("worker queue is full");
        }

        return std::get<1>(std::move(res));
    }

    template <typename TSettings>
    inline Worker& ThreadPool<TSettings>::getWorker()
    {
        size_t id = Worker::getWorkerIdForCurrentThread();

        if(id > m_workers.size())
        {
            id = m_next_worker.fetch_add(1, std::memory_order_relaxed) %
                 m_workers.size();
        }

        return *m_workers[id];
    }
}

#endif
