#ifndef NONCOPYABLE_HPP
#define NONCOPYABLE_HPP

struct noncopyable_t {
    noncopyable_t & operator=(const noncopyable_t &) = delete;
    noncopyable_t(const noncopyable_t &) = delete;
    noncopyable_t() = default;
};

#endif
