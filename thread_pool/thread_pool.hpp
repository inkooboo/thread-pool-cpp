#ifndef thread_pool_hpp
#define thread_pool_hpp

#include <noncopyable.hpp>
#include <vector>
#include <memory>

template <size_t> class worker_t;

class thread_pool_t : private noncopyable_t
{
public:
    enum {AUTODETECT = 0};
    enum {WORKER_QUEUE_SIZE = 1024};

    inline explicit thread_pool_t(size_t threads_count = AUTODETECT);
    inline ~thread_pool_t();

    template <typename Handler>
    inline void post(Handler &&handler);

private:
    size_t m_pool_size;
    size_t m_index;

    typedef std::unique_ptr<worker_t<WORKER_QUEUE_SIZE>> worker_ptr;
    std::vector<worker_ptr> m_pool;
};

#include "thread_pool.ipp"

#endif //thread_pool_hpp

