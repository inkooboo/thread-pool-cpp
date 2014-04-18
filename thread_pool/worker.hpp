#ifndef WORKER_HPP
#define WORKER_HPP

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
class Worker : NonCopyable
{
public:
    typedef FixedFunction<void(), 64> Task;

    /**
     * @brief Worker Constructor.
     * @param queue_size Length of undelaying task queue.
     */
    Worker(size_t queue_size);

    /**
     * @brief ~Worker Destructor.
     * Waits until the executing thread became finished.
     */
    ~Worker();

    /**
     * @brief start Create the executing thread and start tasks execution.
     * @param id Worker ID.
     * @param steal_donor Sibling worker to steal task from it.
     */
    void start(size_t id, Worker *steal_donor);

    /**
     * @brief post Post task to queue.
     * @param handler Handler to be executed in executing thread.
     * @return true on success.
     */
    template <typename Handler>
    bool post(Handler &&handler);

    /**
     * @brief steal Steal one tsak from this worker queue.
     * @param task Place for stealed task to be stored.
     * @return true on success.
     */
    bool steal(Task &task);

    /**
     * @brief get_worker_id_for_this_thread Return worker ID associated with current thread if exists.
     * @return Worker ID.
     */
    static size_t getWorkerIdForCurrentThread();

private:
    /**
     * @brief thread_func Executing thread function.
     * @param id Worker ID to be associated with this thread.
     * @param steal_donor Sibling worker to steal task from it.
     */
    void threadFunc(size_t id, Worker *steal_donor);

    MPMCBoundedQueue<Task> m_queue;
    bool m_stop_flag;
    std::thread m_thread;
    std::atomic<bool> m_starting;
    Worker *m_steal_donor;
};


/// Implementation

inline Worker::Worker(size_t queue_size)
    : m_queue(queue_size)
    , m_stop_flag(false)
{
}

inline Worker::~Worker()
{
    post([&](){m_stop_flag = true;});
    m_thread.join();
}

inline void Worker::start(size_t id, Worker *steal_donor)
{
    m_steal_donor = steal_donor;
    m_starting = true;
    m_thread = std::thread(&Worker::threadFunc, this, id, steal_donor);
    post([&](){m_starting = false;});
    while (m_starting) {
        std::this_thread::yield();
    }
}

inline static size_t * thread_id()
{
    static thread_local size_t tss_id = -1u;
    return &tss_id;
}

inline size_t Worker::getWorkerIdForCurrentThread()
{
    return *thread_id();
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

inline void Worker::threadFunc(size_t id, Worker *steal_donor)
{
    *thread_id() = id;

    Task handler;

    while (!m_stop_flag)
        if (m_queue.pop(handler) || steal_donor->steal(handler)) {
            handler();
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
}

#endif
