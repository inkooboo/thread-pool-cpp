#pragma once

#include <algorithm>
#include <thread>

namespace tp
{

/**
 * @brief The ThreadPoolOptions class provides creation options for
 * ThreadPool.
 */
class ThreadPoolOptions
{
public:
    /**
     * @brief ThreadPoolOptions Construct default options for thread pool.
     */
    ThreadPoolOptions();

    /**
     * @brief setThreadCount Set thread count.
     * @param count Number of threads to be created.
     */
    void setThreadCount(std::size_t count);

    /**
     * @brief setQueueSize Set single worker queue size.
     * @param count Maximum length of queue of single worker.
     */
    void setQueueSize(std::size_t size);

    /**
     * @brief threadCount Return thread count.
     */
    std::size_t threadCount() const;

    /**
     * @brief queueSize Return single worker queue size.
     */
    std::size_t queueSize() const;

private:
    std::size_t m_thread_count;
    std::size_t m_queue_size;
};

/// Implementation

inline ThreadPoolOptions::ThreadPoolOptions()
    : m_thread_count(std::max<std::size_t>(1u, std::thread::hardware_concurrency()))
    , m_queue_size(1024u)
{
}

inline void ThreadPoolOptions::setThreadCount(std::size_t count)
{
    m_thread_count = std::max<std::size_t>(1u, count);
}

inline void ThreadPoolOptions::setQueueSize(std::size_t size)
{
    m_queue_size = std::max<std::size_t>(1u, size);
}

inline std::size_t ThreadPoolOptions::threadCount() const
{
    return m_thread_count;
}

inline std::size_t ThreadPoolOptions::queueSize() const
{
    return m_queue_size;
}

}
