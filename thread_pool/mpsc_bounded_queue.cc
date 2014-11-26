import std.atomic;
import std.type_traits;
import std.cstddef;
import std.vector;
import std.stdexcept;

module thread_pool.mpmc_mounded_queue;

export {

    /**
     * @brief The mpmc_bounded_queue_t class implements bounded multi-producers/multi-consumers lock-free queue.
     * Doesn't accept non-movabe types as T.
     * Inspired by Dmitry Vyukov's mpmc queue.
     * http://www.1024cores.net/home/lock-free-algorithms/queues/bounded-mpmc-queue
     */
    template <typename T>
    class MPMCBoundedQueue {
    public:
        /**
         * @brief MPMCBoundedQueue Constructor.
         * @param size Power of 2 number - queue length.
         * @throws std::invalid_argument if size is bad.
         */
        explicit MPMCBoundedQueue(size_t size);

        /**
         * @brief push Push data to queue.
         * @param data Data to be pushed.
         * @return true on success.
         */
        template <typename U>
        bool push(U &&data);

        /**
         * @brief pop Pop data from queue.
         * @param data Place to store popped data.
         * @return true on sucess.
         */
        bool pop(T &data);

    private:
        MPMCBoundedQueue(const MPMCBoundedQueue&) = delete;
        MPMCBoundedQueue & operator=(const MPMCBoundedQueue&) = delete;

        struct Cell {
            std::atomic<size_t> sequence;
            T data;
        };

        typedef char Cacheline[64];

        Cacheline pad0;
        std::vector<Cell> m_buffer;
        const size_t m_buffer_mask;
        Cacheline pad1;
        std::atomic<size_t> m_enqueue_pos;
        Cacheline pad2;
        std::atomic<size_t> m_dequeue_pos;
        Cacheline pad3;
    };

}


template <typename T>
inline MPMCBoundedQueue<T>::MPMCBoundedQueue(size_t size)
    : m_buffer(size)
    , m_buffer_mask(size - 1)
    , m_enqueue_pos(0)
    , m_dequeue_pos(0)
{
    static_assert(std::is_move_constructible<T>::value, "Should be of movable type");

    bool size_is_power_of_2 = (size >= 2) && ((size & (size - 1)) == 0);
    if (!size_is_power_of_2) {
       throw std::invalid_argument("buffer size should be a power of 2");
    }

    for (size_t i = 0; i < size; ++i)
    {
        m_buffer[i].sequence = i;
    }
}

template <typename T>
template <typename U>
inline bool MPMCBoundedQueue<T>::push(U &&data)
{
    Cell *cell;
    size_t pos = m_enqueue_pos.load(std::memory_order_relaxed);
    for (;;) {
        cell = &m_buffer[pos & m_buffer_mask];
        size_t seq = cell->sequence.load(std::memory_order_acquire);
        intptr_t dif = (intptr_t)seq - (intptr_t)pos;
        if (dif == 0) {
            if (m_enqueue_pos.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed)) {
                break;
            }
        } else if (dif < 0) {
            return false;
        } else {
            pos = m_enqueue_pos.load(std::memory_order_relaxed);
        }
    }

    cell->data = std::forward<U>(data);

    cell->sequence.store(pos + 1, std::memory_order_release);

    return true;
}

template <typename T>
inline bool MPMCBoundedQueue<T>::pop(T &data)
{
    Cell *cell;
    size_t pos = m_dequeue_pos.load(std::memory_order_relaxed);
    for (;;) {
        cell = &m_buffer[pos & m_buffer_mask];
        size_t seq = cell->sequence.load(std::memory_order_acquire);
        intptr_t dif = (intptr_t)seq - (intptr_t)(pos + 1);
        if (dif == 0) {
            if (m_dequeue_pos.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed)) {
                break;
            }
        } else if (dif < 0) {
            return false;
        } else {
            pos = m_dequeue_pos.load(std::memory_order_relaxed);
        }
    }

    data = std::move(cell->data);

    cell->sequence.store(pos + m_buffer_mask + 1, std::memory_order_release);

    return true;
}

