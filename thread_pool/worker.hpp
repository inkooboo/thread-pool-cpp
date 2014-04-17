#ifndef WORKER_HPP
#define WORKER_HPP

#include <fixed_function.hpp>
#include <mpsc_bounded_queue.hpp>
#include <atomic>
#include <thread>

class worker_t : private noncopyable_t
{
public:
    typedef fixed_function_t<void()> task_t;
    enum {QUEUE_SIZE = 1024};

    worker_t();
    ~worker_t();

    void start(size_t id, worker_t *steal_donor);

    template <typename Handler>
    bool post(Handler &&handler);

    bool steal(task_t &task);

    static size_t get_worker_id_for_this_thread();

private:
    void thread_func(size_t id, worker_t *steal_donor);

    mpsc_bounded_queue_t<task_t, QUEUE_SIZE> m_queue;
    bool m_stop_flag;
    std::thread m_thread;
    std::atomic<bool> m_starting;
    worker_t *m_steal_donor;
};


/// Implementation

inline worker_t::worker_t()
    : m_stop_flag(false)
{
}

inline worker_t::~worker_t()
{
    post([&](){m_stop_flag = true;});
    m_thread.join();
}

inline void worker_t::start(size_t id, worker_t *steal_donor)
{
    m_steal_donor = steal_donor;
    m_starting = true;
    m_thread = std::thread(&worker_t::thread_func, this, id, steal_donor);
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

inline size_t worker_t::get_worker_id_for_this_thread()
{
    return *thread_id();
}

template <typename Handler>
inline bool worker_t::post(Handler &&handler)
{
    return m_queue.push(std::forward<Handler>(handler));
}

inline bool worker_t::steal(task_t &task)
{
    return m_queue.pop(task);
}

inline void worker_t::thread_func(size_t id, worker_t *steal_donor)
{
    *thread_id() = id;

    task_t handler;

    while (!m_stop_flag)
        if (m_queue.pop(handler) || steal_donor->steal(handler)) {
            handler();
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
}

#endif
