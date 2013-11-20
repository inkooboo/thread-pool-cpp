#ifndef WORKER_HPP
#define WORKER_HPP

#include <callback.hpp>
#include <mpsc_bounded_queue.hpp>
#include <progressive_waiter.hpp>
#include <atomic>
#include <thread>

class worker_t : private noncopyable_t
{
    enum {QUEUE_SIZE = 1024};
public:
    worker_t();

    ~worker_t();

    size_t task_counter();

    template <typename Handler>
    bool post(Handler &&handler);

private:
    void thread_func();

    typedef callback_t<32> func_t;
    mpsc_bounded_queue_t<func_t, QUEUE_SIZE> m_queue;
    bool m_stop_flag;
    std::thread m_thread;
    std::atomic<bool> m_starting;
    std::atomic<size_t> m_task_counter;
};



/// Implementation

inline worker_t::worker_t()
    : m_stop_flag(false)
    , m_starting(true)
    , m_task_counter(0)
{
    m_thread = std::thread(&worker_t::thread_func, this);
    post([&](){m_starting = false;});
    while(m_starting) {
        std::this_thread::yield();
    }
}

inline worker_t::~worker_t()
{
    post([&](){m_stop_flag = true;});
    m_thread.join();
}

inline size_t worker_t::task_counter() {
    return m_task_counter.load(std::memory_order_relaxed);
}

template <typename Handler>
inline bool worker_t::post(Handler &&handler)
{
    bool rc = m_queue.push(std::forward<Handler>(handler));
    if (rc) {
        m_task_counter.fetch_add(1, std::memory_order_relaxed);
    }
    return rc;
}

inline void worker_t::thread_func()
{
    progressive_waiter_t waiter;

    while (!m_stop_flag) {
        if (func_t *handler = m_queue.front()) {
            waiter.reset();
            try {
                (*handler)();
            }
            catch (...) {
            }
            m_queue.pop();

            m_task_counter.fetch_sub(1, std::memory_order_relaxed);
        }
        else {
            waiter.wait();
        }
    }
}

#endif
