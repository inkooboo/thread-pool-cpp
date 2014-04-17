#ifndef FIXED_FUNCTION_HPP
#define FIXED_FUNCTION_HPP

#include <noncopyable.hpp>
#include <type_traits>
#include <cstring>
#include <stdexcept>
#include <utility>
#include <functional>

template <typename SIGNATURE, int STORAGE_SIZE = 32>
class fixed_function_t;

template <typename R, typename... ARGS, int STORAGE_SIZE>
class fixed_function_t<R(ARGS...), STORAGE_SIZE> : private noncopyable_t {
public:
    fixed_function_t()
        : m_object_ptr(&m_storage)
        , m_method_ptr(nullptr)
        , m_delete_ptr(nullptr)
    {
    }

    template <typename FUNC>
    fixed_function_t(FUNC &&object)
    {
        typedef typename std::remove_reference<FUNC>::type unref_type;

        static_assert(sizeof(unref_type) < STORAGE_SIZE,
                      "functional object don't fit into internal storage");

        m_object_ptr = new (&m_storage) unref_type(std::forward<FUNC>(object));

        m_method_ptr = [](void *object_ptr, ARGS... args) -> R {
            return static_cast<unref_type *>(object_ptr)->operator()(args...);
        };

        m_delete_ptr = [](void *object_ptr) {
            static_cast<unref_type *>(object_ptr)->~unref_type();
        };
    }

    template <typename RET, typename... PARAMS>
    fixed_function_t(RET(*func_ptr)(PARAMS...))
        : fixed_function_t(std::bind(func_ptr))
    {
    }

    fixed_function_t(fixed_function_t &&o)
    {
        move_from_other(o);
    }

    fixed_function_t & operator=(fixed_function_t &&o)
    {
        move_from_other(o);
        return *this;
    }

    ~fixed_function_t()
    {
        if (m_delete_ptr)
            (*m_delete_ptr)(m_object_ptr);
    }

    R operator()(ARGS... args) const
    {
        if (!m_method_ptr)
            throw std::runtime_error("call of empty functor");
        return (*m_method_ptr)(m_object_ptr, args...);
    }

private:
    typename std::aligned_storage<STORAGE_SIZE, sizeof(size_t)>::type m_storage;

    void *m_object_ptr;

    typedef R (*method_type)(void *, ARGS...);
    method_type m_method_ptr;

    typedef void (*delete_type)(void *);
    delete_type m_delete_ptr;

    void move_from_other(fixed_function_t &o)
    {
        this->~fixed_function_t();

        m_method_ptr = o.m_method_ptr;
        m_delete_ptr = o.m_delete_ptr;

        memcpy(&m_storage, &o.m_storage, STORAGE_SIZE);

        o.m_method_ptr = nullptr;
        o.m_delete_ptr = nullptr;
    }
};


#endif
