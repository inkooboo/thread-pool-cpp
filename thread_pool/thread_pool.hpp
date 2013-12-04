#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP

#include <noncopyable.hpp>
#include <worker.hpp>
#include <atomic>
#include <memory>
#include <stdexcept>

class thread_pool_t : private noncopyable_t {
public:
    enum {AUTODETECT = 0};

    explicit thread_pool_t(size_t threads_count = AUTODETECT);

    template <typename Handler>
    void post(Handler &&handler);

private:
    worker_t & get_worker();

    std::unique_ptr<worker_t[]> m_workers;
    size_t m_workers_count;
    std::atomic<size_t> m_next_worker;
};


/// Implementation

inline thread_pool_t::thread_pool_t(size_t threads_count)
    : m_next_worker(0)
{
    if (AUTODETECT == threads_count) {
        threads_count = std::thread::hardware_concurrency();
    }

    if (0 == threads_count) {
        threads_count = 1;
    }

    m_workers_count = threads_count;

    m_workers.reset(new worker_t[m_workers_count]);

    for (size_t i = 0; i < m_workers_count; ++i) {
        m_workers[i].start(i);
    }
}

template <typename Handler>
inline void thread_pool_t::post(Handler &&handler)
{
    if (!get_worker().post(std::forward<Handler>(handler)))
    {
        throw std::overflow_error("worker queue is full");
    }
}

inline worker_t & thread_pool_t::get_worker()
{
    size_t id = worker_t::get_worker_id_for_this_thread();

    if (id > m_workers_count) {
        id = m_next_worker.fetch_add(1, std::memory_order_relaxed) % m_workers_count;
    }

    return m_workers[id];
}

#endif

