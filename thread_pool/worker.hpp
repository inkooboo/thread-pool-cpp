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
    worker_t(int id);

    ~worker_t();

    static int get_id();

    template <typename Handler>
    bool post(Handler &&handler);

private:
    void thread_func(int id);

    typedef fixed_function_t<void()> func_t;
    mpsc_bounded_queue_t<func_t, QUEUE_SIZE> m_queue;
    bool m_stop_flag;
    std::thread m_thread;
    std::atomic<bool> m_starting;
};



/// Implementation

inline worker_t::worker_t(int id)
    : m_stop_flag(false)
    , m_starting(true)
{
    m_thread = std::thread(&worker_t::thread_func, this, id);
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


inline static int * thread_id()
{
    static thread_local int tss_id = -1;
    return &tss_id;
}

inline int worker_t::get_id()
{
    return *thread_id();
}

template <typename Handler>
inline bool worker_t::post(Handler &&handler)
{
    return m_queue.push(std::forward<Handler>(handler));
}

inline void worker_t::thread_func(int id)
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
