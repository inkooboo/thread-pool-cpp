#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP

#include <noncopyable.hpp>
#include <worker.hpp>
#include <memory>
#include <stdexcept>
#include <vector>

class thread_pool_t : private noncopyable_t {
public:
    enum {AUTODETECT = 0};

    explicit thread_pool_t(size_t threads_count = AUTODETECT);

    template <typename Handler>
    void post(Handler &&handler);

private:
    typedef std::unique_ptr<worker_t> worker_ptr;
    typedef std::vector<worker_ptr> pool_t;

    worker_ptr & get_worker();

    pool_t m_pool;
    pool_t::iterator m_next_worker;
};


/// Implementation

inline thread_pool_t::thread_pool_t(size_t threads_count)
{
    if (AUTODETECT == threads_count) {
        threads_count = std::thread::hardware_concurrency();
    }

    if (0 == threads_count) {
        threads_count = 1;
    }

    m_pool.resize(threads_count);

    for (int i = 0; i < (int)m_pool.size(); ++i) {
        m_pool[i].reset(new worker_t(i));
    }

    m_next_worker = m_pool.begin();
}

template <typename Handler>
inline void thread_pool_t::post(Handler &&handler)
{
    worker_ptr &chosen = get_worker();
    if (!chosen->post(std::forward<Handler>(handler)))
    {
        throw std::overflow_error("worker queue is full");
    }
}

inline thread_pool_t::worker_ptr & thread_pool_t::get_worker()
{
    int id = worker_t::get_id();

    if (id != -1 && id < (int)m_pool.size()) {
        return m_pool[id];
    }

    worker_ptr &ret = *m_next_worker;
    ++m_next_worker;
    if (m_next_worker == m_pool.end()) {
        m_next_worker = m_pool.begin();
    }
    return ret;
}

#endif

