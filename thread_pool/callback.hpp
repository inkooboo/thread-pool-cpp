#ifndef CALLBACK_HPP
#define CALLBACK_HPP

#include "noncopyable.hpp"

#include <type_traits>
#include <cstring>

struct callback_t : noncopyable_t
{
    enum {INTERNAL_STORAGE_SIZE = 64};

    callback_t()
        : m_object_ptr(nullptr)
        , m_method_ptr(nullptr)
        , m_delete_ptr(nullptr)
    {}

    template <typename T>
    callback_t(T &&object)
    {
        typedef typename std::remove_reference<T>::type unref_type;

        static_assert(sizeof(unref_type) < INTERNAL_STORAGE_SIZE,
                      "functional object don't fit into internal storage");

        m_object_ptr = new (m_storage) unref_type(std::forward<T>(object));
        m_method_ptr = &method_stub<unref_type>;
        m_delete_ptr = &delete_stub<unref_type>;
    }

    void move_from_other(callback_t &o)
    {
        m_object_ptr = m_storage;
        m_method_ptr = o.m_method_ptr;
        m_delete_ptr = o.m_delete_ptr;

        memcpy(m_storage, o.m_storage, INTERNAL_STORAGE_SIZE);

        o.m_method_ptr = nullptr;
        o.m_delete_ptr = nullptr;
    }

    callback_t(callback_t &&o)
    {
        move_from_other(o);
    }

    callback_t & operator=(callback_t &&o)
    {
        move_from_other(o);
        return *this;
    }

    ~callback_t()
    {
        if (m_delete_ptr)
        {
            (*m_delete_ptr)(m_object_ptr);
        }
    }

    void operator()() const
    {
        if (m_method_ptr)
        {
            (*m_method_ptr)(m_object_ptr);
        }
    }

private:
    typedef void (*method_type)(void *);

    void *m_object_ptr;
    method_type m_method_ptr;
    method_type m_delete_ptr;
    char m_storage[INTERNAL_STORAGE_SIZE];

    template <class T>
    static void method_stub(void *object_ptr)
    {
        T *p = static_cast<T *>(object_ptr);
        p->operator()();
    }

    template <class T>
    static void delete_stub(void *object_ptr)
    {
        T *p = static_cast<T *>(object_ptr);
        p->~T();
    }
};


#endif // CALLBACK_HPP
