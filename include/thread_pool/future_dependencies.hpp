#pragma once

#if defined(THREAD_POOL_USE_BOOST)
#include <boost/thread/future.hpp>
#else
#include <future>
#endif

namespace tp
{
#if defined(THREAD_POOL_USE_BOOST)
    template <typename R>
    using packaged_task_type = boost::packaged_task<R>;
#else
    template <typename R>
    using packaged_task_type = std::packaged_task<R()>;
#endif
}