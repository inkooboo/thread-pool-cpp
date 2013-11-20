#include <mpsc_bounded_queue.hpp>
#include <callback.hpp>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <stdexcept>
#include <thread>

template <size_t QUEUE_SIZE = 1024>
class worker_t : private noncopyable_t
{
public:
    worker_t()
        : m_stop_flag(false)
        , m_sleeping(false)
    {
        m_thread = std::thread(&worker_t::thread_func, this);
        while(!m_sleeping);
    }

    void stop()
    {
        post([&](){m_stop_flag = true;});
        m_thread.join();
    }

    template <typename Handler>
    size_t post(Handler &&handler)
    {
        if (!m_sleeping.load(std::memory_order_acquire)) {
            return m_queue.move_push(std::forward<Handler>(handler));
        }
        std::lock_guard<std::mutex> slock(m_job_mutex);
        size_t rc = m_queue.move_push(std::forward<Handler>(handler));
        if (m_sleeping.load(std::memory_order_acquire)) {
            m_has_job.notify_one();
        }
        return rc;
    }

private:
    typedef callback_t<64> func_t;

    void thread_func()
    {
        while (!m_stop_flag) {
            if (func_t *handler = m_queue.front()) {
                try {
                    (*handler)();
                }
                catch (...) {
                }
                m_queue.pop();
            }
            else {
                std::unique_lock<std::mutex> ulock(m_job_mutex);
                m_sleeping.store(true, std::memory_order_release);
                if (m_queue.front()) {
                    m_sleeping.store(false, std::memory_order_release);
                    continue;
                }
                m_has_job.wait(ulock);
                m_sleeping.store(false, std::memory_order_release);
            }
        }
    }

    mpsc_bounded_queue_t<func_t, QUEUE_SIZE> m_queue;

    bool m_stop_flag;

    std::thread m_thread;

    std::atomic<bool> m_sleeping;
    std::mutex m_job_mutex;
    std::condition_variable m_has_job;
};

inline thread_pool_t::thread_pool_t(size_t threads_count)
    : m_pool_size(threads_count)
    , m_index(0)
{
    if (AUTODETECT == m_pool_size) {
        m_pool_size = std::thread::hardware_concurrency();
    }

    if (0 == m_pool_size) {
        m_pool_size = 1;
    }

    m_pool.resize(m_pool_size);

    for (auto &worker : m_pool) {
        worker.reset(new worker_t<WORKER_QUEUE_SIZE>);
    }
}

inline thread_pool_t::~thread_pool_t()
{
    for (auto &worker : m_pool) {
        worker->stop();
    }
}

template <typename Handler>
inline void thread_pool_t::post(Handler &&handler)
{
    if (-1u == m_pool[m_index++ % m_pool_size]->post(std::forward<Handler>(handler)))
    {
        throw std::overflow_error("worker queue is full");
    }
}


