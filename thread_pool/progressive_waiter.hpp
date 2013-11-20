#ifndef PROGRESSIVE_WAITER_HPP
#define PROGRESSIVE_WAITER_HPP

#include <thread>

class progressive_waiter_t {
public:
    void reset();

    void wait();

private:
    int m_counter = 0;
};


/// Implementation

inline void progressive_waiter_t::reset()
{
    m_counter = 0;
}

inline void progressive_waiter_t::wait()
{
    ++m_counter;
    if (m_counter < 1000) {
        std::this_thread::yield();
    }
    else {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}
#endif
