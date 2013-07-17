#ifndef thread_pool_hpp
#define thread_pool_hpp

#include "noncopyable.hpp"

#include <functional>
#include <vector>
#include <memory>
#include <cstddef>

class thread_pool_t : private noncopyable_t
{
public:
    typedef std::function<void()> task_t;

    enum {AUTODETECT = 0};

    inline explicit thread_pool_t(size_t threads_count = AUTODETECT);
    inline ~thread_pool_t();
    
    inline void post(task_t task);

private:
    size_t m_pool_size;
    size_t m_index;

    class worker_t;
    std::vector<std::unique_ptr<worker_t>> m_pool;
};

#include "thread_pool.ipp"

#endif //thread_pool_hpp

