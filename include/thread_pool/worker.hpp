#pragma once

#include <atomic>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <limits>

namespace tp
{

/**
 * @brief The Worker class owns task queue and executing thread.
 * In thread it tries to pop task from queue. If queue is empty then it tries
 * to steal task from the sibling worker. If steal was unsuccessful then spins
 * with one millisecond delay.
 */
template <typename Task, template<typename> class Queue>
class Worker
{
    using WorkerVector = std::vector<std::unique_ptr<Worker<Task, Queue>>>;

public:
    /**
     * @brief Worker Constructor.
     * @param queue_size Length of undelaying task queue.
     */
    explicit Worker(std::size_t queue_size);

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
     * @param workers Sibling workers for performing round robin work stealing.
     */
    void start(std::size_t id, WorkerVector& workers);

    /**
     * @brief stop Stop all worker's thread and stealing activity.
     * Waits until the executing thread became finished.
     */
    void stop();

    /**
     * @brief tryPost Post task to queue.
     * @param handler Handler to be executed in executing thread.
     * @return true on success.
     */
    template <typename Handler>
    bool tryPost(Handler&& handler);

    /**
     * @brief tryGetLocalTask Get one task from this worker queue.
     * @param task Place for the obtained task to be stored.
     * @return true on success.
     */
    bool tryGetLocalTask(Task& task);

    /**
     * @brief getWorkerIdForCurrentThread Return worker ID associated with
     * current thread if exists.
     * @return Worker ID.
     */
    static std::size_t getWorkerIdForCurrentThread();

private:
    /**
    * @brief tryRoundRobinSteal Try stealing a thread from sibling workers in a round-robin fashion.
    * @param task Place for the obtained task to be stored.
    * @param workers Sibling workers for performing round robin work stealing.
    */
    bool tryRoundRobinSteal(Task& task, WorkerVector& workers);

    /**
     * @brief threadFunc Executing thread function.
     * @param id Worker ID to be associated with this thread.
     * @param workers Sibling workers for performing round robin work stealing.
     */
    void threadFunc(std::size_t id, WorkerVector& workers);

    Queue<Task> m_queue;
    std::atomic<bool> m_running_flag;
    std::thread m_thread;
    std::size_t m_next_donor;
    std::mutex m_conditional_mutex;
    std::condition_variable m_conditional_lock;
    std::atomic<bool> m_ready{false};

};


/// Implementation

namespace detail
{
    inline std::size_t& thread_id()
    {
        static thread_local std::size_t tss_id = std::numeric_limits<std::size_t>::max();
        return tss_id;
    }
}

template <typename Task, template<typename> class Queue>
inline Worker<Task, Queue>::Worker(std::size_t queue_size)
    : m_queue(queue_size)
    , m_running_flag(true)
    , m_next_donor(0) // Initialized in threadFunc.
{
}

template <typename Task, template<typename> class Queue>
inline Worker<Task, Queue>::Worker(Worker&& rhs) noexcept
{
    *this = rhs;
}

template <typename Task, template<typename> class Queue>
inline Worker<Task, Queue>& Worker<Task, Queue>::operator=(Worker&& rhs) noexcept
{
    if (this != &rhs)
    {
        m_queue = std::move(rhs.m_queue);
        m_running_flag = rhs.m_running_flag.load();
        m_thread = std::move(rhs.m_thread);
    }
    return *this;
}

template <typename Task, template<typename> class Queue>
inline void Worker<Task, Queue>::stop()
{
    m_running_flag.store(false, std::memory_order_relaxed);
    {
        std::lock_guard<std::mutex> lock(m_conditional_mutex);
        m_ready.store(true);
    }
    m_conditional_lock.notify_one();
    if(m_thread.joinable()) {
        m_thread.join();
    }
}

template <typename Task, template<typename> class Queue>
inline void Worker<Task, Queue>::start(std::size_t id, WorkerVector& workers)
{
    m_thread = std::thread(&Worker<Task, Queue>::threadFunc, this, id, std::ref(workers));
}

template <typename Task, template<typename> class Queue>
inline std::size_t Worker<Task, Queue>::getWorkerIdForCurrentThread()
{
    return detail::thread_id();
}

template <typename Task, template<typename> class Queue>
template <typename Handler>
inline bool Worker<Task, Queue>::tryPost(Handler&& handler)
{
    {
        std::lock_guard<std::mutex> lock(m_conditional_mutex);
        m_ready.store(true);
    }
    m_conditional_lock.notify_one();
    return m_queue.push(std::forward<Handler>(handler));
}

template <typename Task, template<typename> class Queue>
inline bool Worker<Task, Queue>::tryGetLocalTask(Task& task)
{
    return m_queue.pop(task);
}

template <typename Task, template<typename> class Queue>
inline bool Worker<Task, Queue>::tryRoundRobinSteal(Task& task, WorkerVector& workers)
{
    auto starting_index = m_next_donor;
    // Iterate once through the worker ring, checking for queued work items on each thread.
    do
    {
        // Don't steal from local queue.
        if (m_next_donor != detail::thread_id() && workers[m_next_donor]->tryGetLocalTask(task))
        {
            // Increment before returning so that m_next_donor always points to the worker that has gone the longest
            // without a steal attempt. This helps enforce fairness in the stealing.
            ++m_next_donor %= workers.size();
            return true;
        }
        ++m_next_donor %= workers.size();
    } while (m_next_donor != starting_index);
    return false;
}

template <typename Task, template<typename> class Queue>
inline void Worker<Task, Queue>::threadFunc(std::size_t id, WorkerVector& workers)
{
    detail::thread_id() = id;
    m_next_donor = ++id % workers.size();

    Task handler;

    while (m_running_flag.load(std::memory_order_relaxed))
    {
        // Prioritize local queue, then try stealing from sibling workers.
        if (tryGetLocalTask(handler) || tryRoundRobinSteal(handler, workers))
        {
            try
            {
                handler();
            }
            catch(...)
            {
                // Suppress all exceptions.
            }
        }
        else
        {
            std::unique_lock<std::mutex> lock(m_conditional_mutex);
            m_conditional_lock.wait(lock, [this] { return m_ready.exchange(false); });
        }
    }
}

}
