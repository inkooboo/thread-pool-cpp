#include "mpsc_bounded_queue.hpp"

#include <thread>
#include <stdexcept>

class thread_pool_t::worker_t : private noncopyable_t
{
    enum {QUEUE_SIZE = 1024*1024};
public:
    worker_t()
        : m_stop_flag(false)
        , m_thread(&thread_pool_t::worker_t::thread_func, this)
    {
    }

    void stop()
    {
        m_stop_flag = true;
        m_thread.join();
    }

    bool post(task_t &&task)
    {
        return m_queue.move_push(std::move(task));
    }

private:
    void thread_func()
    {
        task_t task;
        while (!m_stop_flag)
        {
            if (m_queue.move_pop(task))
            {
                task();
            }
            else
            {
                std::this_thread::yield();
            }
        }
    }

    mpsc_bounded_queue_t<task_t, QUEUE_SIZE> m_queue;

    bool m_stop_flag;
    std::thread m_thread;
};

inline thread_pool_t::thread_pool_t(size_t threads_count)
    : m_pool_size(threads_count)
    , m_index(0)
{
    if (AUTODETECT == m_pool_size)
        m_pool_size = std::thread::hardware_concurrency();

    if (0 == m_pool_size)
        m_pool_size = 1;

    m_pool.resize(m_pool_size);

    for (auto &worker : m_pool)
    {
        worker.reset(new worker_t);
    }
}

inline thread_pool_t::~thread_pool_t()
{
    for (auto &worker : m_pool)
    {
        worker->stop();
    }
}

inline void thread_pool_t::post(task_t task)
{
    if (!m_pool[m_index++ % m_pool_size]->post(std::move(task)))
    {
        throw std::overflow_error("worker queue is full");
    }
}


