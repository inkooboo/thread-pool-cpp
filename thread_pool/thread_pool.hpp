#ifndef thread_pool_hpp
#define thread_pool_hpp

#include "noncopyable.hpp"

#include <vector>
#include <cstddef>

class thread_pool_t : private noncopyable_t
{
public:
    enum {AUTODETECT = 0};

    inline explicit thread_pool_t(size_t threads_count = AUTODETECT);
    inline ~thread_pool_t();

    template <typename Handler>
    inline void post(Handler &&handler);

private:
    size_t m_pool_size;
    size_t m_index;

    class worker_t;
    std::vector<worker_t *> m_pool;
};

#include "thread_pool.ipp"

#endif //thread_pool_hpp

