#ifndef WORKER_HPP
#define WORKER_HPP

// http://stackoverflow.com/a/25393790/5724090
// Static/global variable exists in a per-thread context (thread local storage).
#if defined (__GNUC__)
    #define ATTRIBUTE_TLS __thread
#elif defined (_MSC_VER)
    #define ATTRIBUTE_TLS __declspec(thread)
#else // !__GNUC__ && !_MSC_VER
    #error "Define a thread local storage qualifier for your compiler/platform!"
#endif

#include <fixed_function.hpp>
#include <mpsc_bounded_queue.hpp>
#include <atomic>
#include <thread>

/**
 * @brief The Worker class owns task queue and executing thread.
 * In executing thread it tries to pop task from queue. If queue is empty
 * then it tries to steal task from the sibling worker. If stealing was unsuccessful
 * then spins with one millisecond delay.
 */
class Worker {
public:
    typedef FixedFunction<void(), 128> Task;
    
    using OnStart = std::function<void(size_t id)>;
    using OnStop = std::function<void(size_t id)>;

    /**
     * @brief Worker Constructor.
     * @param queue_size Length of undelaying task queue.
     */
    explicit Worker(size_t queue_size);

    /**
     * @brief start Create the executing thread and start tasks execution.
     * @param id Worker ID.
     * @param steal_donor Sibling worker to steal task from it.
     * @param onStart A handler which is executed when each thread pool thread starts
     * @param onStop A handler which is executed when each thread pool thread stops
     */
    void start(size_t id, Worker *steal_donor, OnStart onStart, OnStop onStop);

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
    bool post(Handler &&handler);

    /**
     * @brief steal Steal one task from this worker queue.
     * @param task Place for stealed task to be stored.
     * @return true on success.
     */
    bool steal(Task &task);

    /**
     * @brief getWorkerIdForCurrentThread Return worker ID associated with current thread if exists.
     * @return Worker ID.
     */
    static size_t getWorkerIdForCurrentThread();

private:
    Worker(const Worker&) = delete;
    Worker & operator=(const Worker&) = delete;
    
    /**
     * @brief thread_id Return worker ID associated with current thread if exists.
     * @return Worker ID.
     */
    static size_t * thread_id();

    /**
     * @brief threadFunc Executing thread function.
     * @param id Worker ID to be associated with this thread.
     * @param steal_donor Sibling worker to steal task from it.
     * @param onStart A handler which is executed when each thread pool thread starts
     * @param onStop A handler which is executed when each thread pool thread stops
     */
    void threadFunc(size_t id, Worker *steal_donor, OnStart onStart, OnStop onStop);

    MPMCBoundedQueue<Task> m_queue;
    std::atomic<bool> m_running_flag;
    std::thread m_thread;
};


/// Implementation

namespace detail {
    inline size_t * thread_id()
    {
        static ATTRIBUTE_TLS size_t tss_id = -1u;
        return &tss_id;
    }
}

inline Worker::Worker(size_t queue_size)
    : m_queue(queue_size)
    , m_running_flag(true)
{
}

inline void Worker::stop()
{
    m_running_flag.store(false, std::memory_order_relaxed);
    m_thread.join();
}

inline void Worker::start(size_t id, Worker *steal_donor, OnStart onStart, OnStop onStop)
{
    m_thread = std::thread(&Worker::threadFunc, this, id, steal_donor, onStart, onStop);
}

inline size_t Worker::getWorkerIdForCurrentThread()
{
    return *detail::thread_id();
}

template <typename Handler>
inline bool Worker::post(Handler &&handler)
{
    return m_queue.push(std::forward<Handler>(handler));
}

inline bool Worker::steal(Task &task)
{
    return m_queue.pop(task);
}

inline void Worker::threadFunc(size_t id, Worker *steal_donor, OnStart onStart, OnStop onStop)
{
    *detail::thread_id() = id;
    
    if (onStart) {
        try { onStart(id); } catch (...) {}
    }

    Task handler;

    while (m_running_flag.load(std::memory_order_relaxed))
        if (m_queue.pop(handler) || steal_donor->steal(handler)) {
            try {handler();} catch (...) {}
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        
    if (onStop) {
        try { onStop(id); } catch (...) {}
        }
}

#endif
