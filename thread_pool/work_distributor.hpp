#ifndef WORK_DISTRIBUTOR_HPP
#define WORK_DISTRIBUTOR_HPP

#include <worker.hpp>
#include <vector>
#include <atomic>
#include <map>

class work_distributor_t {
public:
    void add_worker(worker_t *worker);

    worker_t * get_worker();

private:
    typedef std::map<std::thread::id, worker_t *> workers_map_t;
    workers_map_t m_workers;
    workers_map_t::iterator m_next_worker;
};


/// Implementation

inline void work_distributor_t::add_worker(worker_t *worker)
{
    m_workers[worker->thread_id()] = worker;
    m_next_worker = m_workers.begin();
}

inline static std::thread::id cached_thread_id()
{
    static const std::thread::id dummy;
    static thread_local std::thread::id cached;
    if (cached == dummy) {
        cached = std::this_thread::get_id();
    }
    return cached;
}

inline worker_t * work_distributor_t::get_worker()
{
    workers_map_t::iterator found = m_workers.find(cached_thread_id());
    if (found != m_workers.end()) {
        return found->second;
    }

    worker_t *ret = m_next_worker->second;
    ++m_next_worker;
    if (m_next_worker == m_workers.end()) {
        m_next_worker = m_workers.begin();
    }
    return ret;
}

#endif
