#ifndef mpsc_queue_h
#define mpsc_queue_h

#include "noncopyable.hpp"

#include <atomic>
#include <type_traits>
#include <cstddef>

/// Inspired by
/// http://www.1024cores.net/home/lock-free-algorithms/queues/bounded-mpmc-queue

template <typename T, unsigned BUFFER_SIZE>
class mpsc_bounded_queue_t : private noncopyable_t
{
    enum {BUFFER_MASK = BUFFER_SIZE - 1};
public:
    mpsc_bounded_queue_t()
        : m_enqueue_pos(0)
        , m_dequeue_pos(0)
    {
        static_assert(std::is_move_constructible<T>::value, "Should be movable type");
        static_assert((BUFFER_SIZE >= 2) && ((BUFFER_SIZE & (BUFFER_SIZE - 1)) == 0),
                      "buffer size should be a power of 2");

        for (size_t i = 0; i < BUFFER_SIZE; ++i)
        {
            m_buffer[i].sequence = i;
        }
    }

    bool move_push(T &&data)
    {
        cell_t *cell = nullptr;
        size_t pos = m_enqueue_pos.load(std::memory_order_relaxed);
        for (;;)
        {
            cell = &m_buffer[pos & BUFFER_MASK];
            size_t seq = cell->sequence.load(std::memory_order_acquire);
            intptr_t dif = (intptr_t)seq - (intptr_t)pos;
            if (dif == 0)
            {
                if (m_enqueue_pos.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed))
                    break;
            }
            else if (dif < 0)
                return false;
            else
                pos = m_enqueue_pos.load(std::memory_order_relaxed);
        }
        cell->data = std::move(data);
        cell->sequence.store(pos + 1, std::memory_order_release);
        return true;
    }

    bool move_pop(T &data)
    {
        cell_t &cell = m_buffer[m_dequeue_pos & BUFFER_MASK];
        size_t seq = cell.sequence.load(std::memory_order_acquire);
        intptr_t dif = (intptr_t)seq - (intptr_t)(m_dequeue_pos + 1);
        if (dif == 0)
        {
            ++m_dequeue_pos;
            data = std::move(cell.data);
            cell.sequence.store(m_dequeue_pos + BUFFER_MASK, std::memory_order_release);
            return true;
        }

        return false;
    }

private:
    struct cell_t
    {
        std::atomic<size_t>   sequence;
        T                     data;
    };

    typedef char cacheline_pad_t[64];

    cacheline_pad_t  pad0;
    cell_t m_buffer[BUFFER_SIZE];
    cacheline_pad_t  pad1;
    std::atomic<size_t> m_enqueue_pos;
    cacheline_pad_t pad2;
    size_t m_dequeue_pos;
    cacheline_pad_t pad3;
};

#endif
