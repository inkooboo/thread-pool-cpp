#include <worker.hpp>

size_t * Worker::thread_id()
{        
    static thread_local size_t tss_id = -1u;    
    return &tss_id;
}
