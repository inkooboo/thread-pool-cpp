#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP

#include <noncopyable.hpp>
#include <worker.hpp>
#include <atomic>
#include <memory>
#include <stdexcept>
#include <vector>

class thread_pool_t : private noncopyable_t {
public:
    enum {AUTODETECT = 0};

    explicit thread_pool_t(size_t threads_count = AUTODETECT);

    ~thread_pool_t();

    template <typename Handler>
    void post(Handler &&handler);

private:
    typedef std::vector<worker_t *> pool_t;

    worker_t * get_worker();

    pool_t m_pool;
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

    m_pool.resize(threads_count);

    for (size_t i = 0; i < m_pool.size(); ++i) {
        m_pool[i] = new worker_t(i);
    }
}

inline thread_pool_t::~thread_pool_t()
{
    for (auto &worker : m_pool) {
        delete worker;
    }
}

template <typename Handler>
inline void thread_pool_t::post(Handler &&handler)
{
    if (!get_worker()->post(std::forward<Handler>(handler)))
    {
        throw std::overflow_error("worker queue is full");
    }
}

inline worker_t *thread_pool_t::get_worker()
{
    int id = worker_t::get_id();

    if (id > m_pool.size()) {
        id = m_next_worker.fetch_add(1, std::memory_order_relaxed) % m_pool.size();
    }

    return m_pool[id];
}

#endif

