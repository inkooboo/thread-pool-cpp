#pragma once

// Use C++11 thread_local specifier when available, otherwise revert to platforms
// specific implementations.
//
// http://stackoverflow.com/a/25393790/5724090
// Static/global variable exists in a per-thread context (thread local storage).
#if THREAD_POOL_HAS_THREAD_LOCAL_STORAGE
    #define ATTRIBUTE_TLS thread_local
#elif defined (__GNUC__)
    #define ATTRIBUTE_TLS __thread
#elif defined (_MSC_VER)
    #define ATTRIBUTE_TLS __declspec(thread)
#else // !__GNUC__ && !_MSC_VER
    #error "Define a thread local storage qualifier for your compiler/platform!"
#endif

#include <atomic>
#include <thread>
#include "./fixed_function.hpp"
#include "./mpsc_bounded_queue.hpp"

namespace tp
{
    /**
     * @brief The Worker class owns task queue and executing thread.
     * In executing thread it tries to pop task from queue. If queue is empty
     * then it tries to steal task from the sibling worker. If stealing was
     * unsuccessful
     * then spins with one millisecond delay.
     */

    template <std::size_t TASK_SIZE=128>
    class Worker
    {
    public:
        using Task = FixedFunction<void(), TASK_SIZE>;

        /**
         * @brief Worker Constructor.
         * @param queue_size Length of undelaying task queue.
         */
        explicit Worker(std::size_t queue_size);

        /**
         * @brief start Create the executing thread and start tasks execution.
         * @param id Worker ID.
         * @param steal_donor Sibling worker to steal task from it.
         */
        void start(std::size_t id, Worker* steal_donor);

        /**
         * @brief stop Stop all worker's thread and stealing activity.
         * Waits until the executing thread became finished.
         */
        void stop();

        /**
         * @brief post Post task to queue.
         * @param handler Handler to be executed in executing thread.
         * @return true on success.
         */
        template <typename Handler>
        inline bool post(Handler&& handler);

        /**
         * @brief steal Steal one task from this worker queue.
         * @param task Place for stealed task to be stored.
         * @return true on success.
         */
        bool steal(Task& task);

        /**
         * @brief getWorkerIdForCurrentThread Return worker ID associated with
         * current thread if exists.
         * @return Worker ID.
         */
        static std::size_t getWorkerIdForCurrentThread();

    private:
        Worker(const Worker<TASK_SIZE>&) = delete;
        Worker<TASK_SIZE>& operator=(const Worker<TASK_SIZE>&) = delete;

    public:
        Worker(Worker<TASK_SIZE>&& rhs)
            : m_queue(std::move(rhs.m_queue)),
              m_running_flag(rhs.m_running_flag.load()),
              m_thread(std::move(rhs.m_thread))
        {
        }

        Worker<TASK_SIZE>& operator=(Worker<TASK_SIZE>&& rhs)
        {
            m_queue = std::move(rhs.m_queue);
            m_running_flag = rhs.m_running_flag.load();
            m_thread = std::move(rhs.m_thread);

            return *this;
        }

    private:
        /**
         * @brief threadFunc Executing thread function.
         * @param id Worker ID to be associated with this thread.
         * @param steal_donor Sibling worker to steal task from it.
         */
        void threadFunc(std::size_t id, Worker<TASK_SIZE>* steal_donor);

        MPMCBoundedQueue<Task> m_queue;
        std::atomic<bool> m_running_flag;
        std::thread m_thread;
    };


    /// Implementation

    namespace detail
    {
        inline std::size_t* thread_id()
        {
            static ATTRIBUTE_TLS std::size_t tss_id = -1u;
            return &tss_id;
        }
    }

    template <std::size_t TASK_SIZE>
    inline Worker<TASK_SIZE>::Worker(std::size_t queue_size)
        : m_queue(queue_size), m_running_flag(true)
    {
    }

    template <std::size_t TASK_SIZE>    
    inline void Worker<TASK_SIZE>::stop()
    {
        m_running_flag.store(false, std::memory_order_relaxed);
        m_thread.join();
    }

    template <std::size_t TASK_SIZE>
    inline void Worker<TASK_SIZE>::start(std::size_t id, Worker* steal_donor)
    {
        m_thread = std::thread(&Worker<TASK_SIZE>::threadFunc, this, id, steal_donor);
    }

    template <std::size_t TASK_SIZE>
    inline std::size_t Worker<TASK_SIZE>::getWorkerIdForCurrentThread()
    {
        return *detail::thread_id();
    }

    template <std::size_t TASK_SIZE>
    template <typename Handler>
    inline bool Worker<TASK_SIZE>::post(Handler&& handler)
    {
        return m_queue.push(std::forward<Handler>(handler));
    }
    
    template <std::size_t TASK_SIZE>    
    inline bool Worker<TASK_SIZE>::steal(Task& task)
    {
        return m_queue.pop(task);
    }

    template <std::size_t TASK_SIZE>    
    inline void Worker<TASK_SIZE>::threadFunc(std::size_t id, Worker* steal_donor)
    {
        *detail::thread_id() = id;

        Task handler;

        while(m_running_flag.load(std::memory_order_relaxed))
        {
            if(m_queue.pop(handler) || steal_donor->steal(handler))
            {
                try
                {
                    handler();
                }
                catch(...)
                {
                }
            }
            else
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
    }
}
