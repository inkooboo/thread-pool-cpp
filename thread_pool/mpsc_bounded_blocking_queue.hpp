#ifndef MPSC_QUEUE_HPP
#define MPSC_QUEUE_HPP

#include <noncopyable.hpp>

#include <atomic>
#include <cstddef>
#include <mutex>
#include <type_traits>

template <typename T, unsigned BUFFER_SIZE>
class mpsc_bounded_queue_t : private noncopyable_t {
public:
    mpsc_bounded_queue_t();

    template <typename U>
    bool push(U &&data);

    T * front();

    void pop();

private:
    struct cell_t {
        std::atomic<bool> free;
        T data;
    };

    size_t m_enqueue_pos;
    size_t m_dequeue_pos;

    cell_t m_buffer[BUFFER_SIZE];

    std::mutex m_mutex;
};


/// Implementation

template <typename T, unsigned BUFFER_SIZE>
inline mpsc_bounded_queue_t<T, BUFFER_SIZE>::mpsc_bounded_queue_t()
    : m_enqueue_pos(0)
    , m_dequeue_pos(0)
{
    static_assert(std::is_move_constructible<T>::value, "Should be of movable type");

    for (auto &cell : m_buffer) {
        cell.free = true;
    }
}

template <typename T, unsigned BUFFER_SIZE>
template <typename U>
inline bool mpsc_bounded_queue_t<T, BUFFER_SIZE>::push(U &&data)
{
    cell_t *cell = nullptr;
    {
        std::lock_guard<std::mutex> slock(m_mutex);
        size_t enqueue_pos = m_enqueue_pos;
        cell = &m_buffer[enqueue_pos % BUFFER_SIZE];

        if (!cell->free) {
            return false;
        }

        ++m_enqueue_pos;
    }

    cell->free = false;

    cell->data = std::forward<U>(data);
    return true;
}

template <typename T, unsigned BUFFER_SIZE>
inline T * mpsc_bounded_queue_t<T, BUFFER_SIZE>::front()
{
    cell_t *cell = &m_buffer[m_dequeue_pos % BUFFER_SIZE];
    if (cell->free) {
        return nullptr;
    }
    return &cell->data;
}

template <typename T, unsigned BUFFER_SIZE>
inline void mpsc_bounded_queue_t<T, BUFFER_SIZE>::pop()
{
    cell_t *cell = &m_buffer[m_dequeue_pos++ % BUFFER_SIZE];
    cell->free = true;
}

#endif
