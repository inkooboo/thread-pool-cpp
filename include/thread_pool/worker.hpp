#pragma once

#include <thread_pool/fixed_function.hpp>
#include <thread_pool/mpsc_bounded_queue.hpp>

#include <atomic>
#include <thread>

namespace tp
{

/**
 * @brief The Worker class owns task queue and executing thread.
 * In executing thread it tries to pop task from queue. If queue is empty
 * then it tries to steal task from the sibling worker. If stealing was
 * unsuccessful
 * then spins with one millisecond delay.
 */
template <size_t TASK_SIZE>
class Worker
{
public:
    using Task = FixedFunction<void(), TASK_SIZE>;

    /**
     * @brief Worker Constructor.
     * @param queue_size Length of undelaying task queue.
     */
    explicit Worker(size_t queue_size);

    /**
     * @brief Move ctor implementation.
     */
    Worker(Worker&& rhs) noexcept;

    /**
     * @brief Move assignment implementaion.
     */
    Worker& operator=(Worker&& rhs) noexcept;

    /**
     * @brief start Create the executing thread and start tasks execution.
     * @param id Worker ID.
     * @param steal_donor Sibling worker to steal task from it.
     */
    void start(size_t id, Worker* steal_donor);

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
    bool post(Handler&& handler);

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
    static size_t getWorkerIdForCurrentThread();

private:
    /**
     * @brief threadFunc Executing thread function.
     * @param id Worker ID to be associated with this thread.
     * @param steal_donor Sibling worker to steal task from it.
     */
    void threadFunc(size_t id, Worker* steal_donor);

    MPMCBoundedQueue<Task> m_queue;
    std::atomic<bool> m_running_flag;
    std::thread m_thread;
};


/// Implementation

namespace detail
{
    inline size_t* thread_id()
    {
        static thread_local size_t tss_id = -1u;
        return &tss_id;
    }
}

template <size_t TASK_SIZE>
inline Worker<TASK_SIZE>::Worker(size_t queue_size)
    : m_queue(queue_size), m_running_flag(true)
{
}

template <size_t TASK_SIZE>
inline Worker<TASK_SIZE>::Worker(Worker&& rhs) noexcept
{
    *this = rhs;
}

template <size_t TASK_SIZE>
inline Worker<TASK_SIZE>& Worker<TASK_SIZE>::operator=(Worker&& rhs) noexcept
{
    if (this != &rhs)
    {
        m_queue = std::move(rhs.m_queue);
        m_running_flag = rhs.m_running_flag.load();
        m_thread = std::move(rhs.m_thread);
    }
    return *this;
}

template <size_t TASK_SIZE>
inline void Worker<TASK_SIZE>::stop()
{
    m_running_flag.store(false, std::memory_order_relaxed);
    m_thread.join();
}

template <size_t TASK_SIZE>
inline void Worker<TASK_SIZE>::start(size_t id, Worker* steal_donor)
{
    m_thread = std::thread(&Worker<TASK_SIZE>::threadFunc, this, id, steal_donor);
}

template <size_t TASK_SIZE>
inline size_t Worker<TASK_SIZE>::getWorkerIdForCurrentThread()
{
    return *detail::thread_id();
}

template <size_t TASK_SIZE>
template <typename Handler>
inline bool Worker<TASK_SIZE>::post(Handler&& handler)
{
    return m_queue.push(std::forward<Handler>(handler));
}

template <size_t TASK_SIZE>
inline bool Worker<TASK_SIZE>::steal(Task& task)
{
    return m_queue.pop(task);
}

template <size_t TASK_SIZE>
inline void Worker<TASK_SIZE>::threadFunc(size_t id, Worker* steal_donor)
{
    *detail::thread_id() = id;

    Task handler;

    while (m_running_flag.load(std::memory_order_relaxed))
    {
        if (m_queue.pop(handler) || steal_donor->steal(handler))
        {
            try
            {
                handler();
            }
            catch(...)
            {
                // supress all exceptions
            }
        }
        else
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}

}
