#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP

#include <noncopyable.hpp>
#include <worker.hpp>
#include <work_distributor.hpp>
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
    std::vector<worker_ptr> m_pool;

    work_distributor_t m_distributor;
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

    for (auto &worker : m_pool) {
        worker.reset(new worker_t);
        m_distributor.add_worker(worker.get());
    }
}

template <typename Handler>
inline void thread_pool_t::post(Handler &&handler)
{
    worker_t *chosen = m_distributor.get_worker();
    if (!chosen->post(std::forward<Handler>(handler)))
    {
        throw std::overflow_error("worker queue is full");
    }
}

#endif

