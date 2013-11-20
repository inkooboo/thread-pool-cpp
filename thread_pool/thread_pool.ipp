#include <mpsc_bounded_queue.hpp>
#include <callback.hpp>
#include <stdexcept>
#include <thread>

template <size_t QUEUE_SIZE = 1024>
class worker_t : private noncopyable_t
{
public:
    worker_t()
        : m_stop_flag(false)
        , m_starting(true)
    {
        m_thread = std::thread(&worker_t::thread_func, this);
        post([&](){m_starting = false;});
        while(m_starting) {
            std::this_thread::yield();
        }
    }

    void stop()
    {
        post([&](){m_stop_flag = true;});
        m_thread.join();
    }

    template <typename Handler>
    bool post(Handler &&handler)
    {
        return m_queue.push(std::forward<Handler>(handler));
    }

private:
    typedef callback_t<32> func_t;

    struct progressive_waiter_t {
        void reset() {
            m_counter = 0;
        }

        void wait() {
            ++m_counter;
            if (m_counter < 1000)
            {
                std::this_thread::yield();
            }
            else {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }

    private:
        int m_counter = 0;
    };

    void thread_func()
    {
        progressive_waiter_t waiter;

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

    mpsc_bounded_queue_t<func_t, QUEUE_SIZE> m_queue;

    bool m_stop_flag;

    std::thread m_thread;

    std::atomic<bool> m_starting;
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
    if (!m_pool[m_index.fetch_add(1, std::memory_order_relaxed) % m_pool_size]->post(std::forward<Handler>(handler)))
    {
        throw std::overflow_error("worker queue is full");
    }
}


