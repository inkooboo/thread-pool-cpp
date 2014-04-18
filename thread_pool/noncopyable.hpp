#ifndef NONCOPYABLE_HPP
#define NONCOPYABLE_HPP

/**
 * @brief The NonCopyable struct implements non-copyable idiom.
 */
struct NonCopyable {
    NonCopyable & operator=(const NonCopyable &) = delete;
    NonCopyable(const NonCopyable &) = delete;
    NonCopyable() = default;
};

#endif
