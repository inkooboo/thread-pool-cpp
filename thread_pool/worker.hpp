#ifndef WORKER_HPP
#define WORKER_HPP

#include <fixed_function.hpp>
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

    void start(size_t id);

    static size_t get_worker_id_for_this_thread();

    template <typename Handler>
    bool post(Handler &&handler);

private:
    void thread_func(size_t id);

    typedef fixed_function_t<void()> func_t;
    mpsc_bounded_queue_t<func_t, QUEUE_SIZE> m_queue;
    bool m_stop_flag;
    std::thread m_thread;
    std::atomic<bool> m_starting;
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

inline void worker_t::start(size_t id)
{
    m_starting = true;
    m_thread = std::thread(&worker_t::thread_func, this, id);
    post([&](){m_starting = false;});
    while(m_starting) {
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

inline void worker_t::thread_func(size_t id)
{
    progressive_waiter_t waiter;

    *thread_id() = id;

    while (!m_stop_flag) {
        if (func_t *handler = m_queue.front()) {
            waiter.reset();
            try {
                (*handler)();
            }
            catch (...) {
            }
            m_queue.pop();
        }
        else {
            waiter.wait();
        }
    }
}

#endif
