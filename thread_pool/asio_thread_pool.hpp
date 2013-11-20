#ifndef ASIO_THREAD_POOL_HPP
#define ASIO_THREAD_POOL_HPP

#include <boost/asio.hpp>

#include <functional>
#include <thread>
#include <vector>
#include <memory>

class asio_thread_pool_t
{
public:
    inline asio_thread_pool_t(size_t threads);

    inline ~asio_thread_pool_t()
    {
        stop();
    }

    inline void join_thread_pool();

    template <typename Handler>
    inline void post(Handler &&handler)
    {
        m_io_svc.post(handler);
    }

private:
    inline void start();
    inline void stop();
    inline void worker_thread_func();

    boost::asio::io_service m_io_svc;
    std::unique_ptr<boost::asio::io_service::work> m_work;

    std::vector<std::thread> m_threads;
};

inline asio_thread_pool_t::asio_thread_pool_t(size_t threads)
    : m_threads(threads)
{
    start();
}

inline void asio_thread_pool_t::start()
{
    m_work.reset(new boost::asio::io_service::work(m_io_svc));

    for (auto &i : m_threads)
    {
        i = std::thread(&asio_thread_pool_t::worker_thread_func, this);
    }

}

inline void asio_thread_pool_t::stop()
{
    m_work.reset();

    m_io_svc.stop();

    for (auto &i : m_threads)
    {
        if (i.joinable())
        {
            i.join();
        }
    }
}

inline void asio_thread_pool_t::join_thread_pool()
{
    m_io_svc.run();
}

inline void asio_thread_pool_t::worker_thread_func()
{
    join_thread_pool();
}

#endif
