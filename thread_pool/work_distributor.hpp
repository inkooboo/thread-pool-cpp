#ifndef WORK_DISTRIBUTOR_HPP
#define WORK_DISTRIBUTOR_HPP

#include <worker.hpp>
#include <vector>
#include <atomic>

class work_distributor_t {
public:
    work_distributor_t();

    void add_worker(worker_t *worker);

    worker_t * get_worker();

private:
    std::atomic<size_t> m_index;
    size_t m_worker_count;
    std::vector<worker_t *> m_workers;
};


/// Implementation

inline work_distributor_t::work_distributor_t()
    : m_index(0)
    , m_worker_count(0)
{
}

inline void work_distributor_t::add_worker(worker_t *worker)
{
    ++m_worker_count;
    m_workers.push_back(worker);
}

inline worker_t * work_distributor_t::get_worker() {
    size_t index = m_index.fetch_add(1, std::memory_order_relaxed);
    worker_t *w1 = m_workers[index % m_worker_count];
    worker_t *w2 = m_workers[(index + 1) % m_worker_count];

    return w1->task_counter() < w2->task_counter() ? w1 : w2;
}

#endif
