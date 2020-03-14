#include <gtest/gtest.h>

#include <thread_pool/fixed_function.hpp>

#include <string>
#include <type_traits>
#include <functional>

int test_free_func(int i)
{
    return i;
}

template <typename T>
T test_free_func_template(T p)
{
    return p;
}

void test_void(int &p, int v)
{
    p = v;
}

struct A {
    int b(const int &p)
    {
        return p;
    }
    void c(int &i)
    {
        i = 43;
    }
};

template <typename T>
struct Foo
{
    template <typename U>
    U bar(U p)
    {
        return p + payload;
    }

    T payload;
};

template <typename T>
void print_overhead()
{
    using func_type = tp::FixedFunction<void(), sizeof(T)>;
    int t_s = sizeof(T);
    int f_s = sizeof(func_type);
    std::cout << " - for type size " << t_s << "\n"
              << "    function size is " << f_s << "\n"
              << "    overhead is " << float(f_s - t_s)/t_s * 100 << "%\n";
}

static std::string str_fun()
{
    return "123";
}

TEST(FixedFunction, Overhead)
{
    print_overhead<char[8]>();
    print_overhead<char[16]>();
    print_overhead<char[32]>();
    print_overhead<char[64]>();
    print_overhead<char[128]>();
}

TEST(FixedFunction, allocDealloc)
{
    static size_t def = 0;
    static size_t cop = 0;
    static size_t mov = 0;
    static size_t cop_ass = 0;
    static size_t mov_ass = 0;
    static size_t destroyed = 0;
    struct cnt {
        std::string payload;
        cnt() { def++; }
        cnt(const cnt &o) { payload = o.payload; cop++;}
        cnt(cnt &&o) { payload = std::move(o.payload); mov++;}
        cnt & operator=(const cnt &o) { payload = o.payload; cop_ass++; return *this; }
        cnt & operator=(cnt &&o) { payload = std::move(o.payload); mov_ass++; return *this; }
        ~cnt() { destroyed++; }
        std::string operator()() { return payload; }
    };

    {
        cnt c1;
        c1.payload = "xyz";
        tp::FixedFunction<std::string()> f1(c1);
        ASSERT_EQ(std::string("xyz"), f1());

        tp::FixedFunction<std::string()> f2;
        f2 = std::move(f1);
        ASSERT_EQ(std::string("xyz"), f2());

        tp::FixedFunction<std::string()> f3(std::move(f2));
        ASSERT_EQ(std::string("xyz"), f3());

        tp::FixedFunction<std::string()> f4(str_fun);
        ASSERT_EQ(std::string("123"), f4());

        f4 = std::move(f3);
        ASSERT_EQ(std::string("xyz"), f4());

        cnt c2;
        c2.payload = "qwe";
        f4 = std::move(tp::FixedFunction<std::string()>(c2));
        ASSERT_EQ(std::string("qwe"), f4());
    }

    ASSERT_EQ(def + cop + mov, destroyed);
    ASSERT_EQ(2llu, def);
    ASSERT_EQ(0llu, cop);
    ASSERT_EQ(6llu, mov);
    ASSERT_EQ(0llu, cop_ass);
    ASSERT_EQ(0llu, mov_ass);
}

TEST(FixedFunction, freeFunc)
{
    tp::FixedFunction<int(int)> f(test_free_func);
    auto f1 = std::move(f);
    ASSERT_EQ(3, f1(3));
};

TEST(FixedFunction, freeFuncTemplate)
{
    tp::FixedFunction<std::string(std::string)> f(test_free_func_template<std::string>);
    auto f1 = std::move(f);
    ASSERT_EQ(std::string("abc"), f1("abc"));
}


TEST(FixedFunction, voidFunc)
{
    tp::FixedFunction<void(int &, int)> f(test_void);
    auto f1 = std::move(f);
    int p = 0;
    f1(p, 42);
    ASSERT_EQ(42, p);
}

TEST(FixedFunction, classMethodVoid)
{
    using namespace std::placeholders;
    A a;
    int i = 0;
    tp::FixedFunction<void(int &)> f(std::bind(&A::c, &a, _1));
    auto f1 = std::move(f);
    f1(i);
    ASSERT_EQ(43, i);
}

TEST(FixedFunction, classMethod1)
{
    using namespace std::placeholders;
    A a;
    tp::FixedFunction<int(const int&)> f(std::bind(&A::b, &a, _1));
    auto f1 = std::move(f);
    ASSERT_EQ(4, f1(4));
}

TEST(FixedFunction, classMethod2)
{
    using namespace std::placeholders;
    Foo<float> foo;
    foo.payload = 1.f;
    tp::FixedFunction<int(int)> f(std::bind(&Foo<float>::bar<int>, &foo, _1));
    auto f1 = std::move(f);
    ASSERT_EQ(2, f1(1));
}

TEST(FixedFunction, lambda)
{
    const std::string s1 = "s1";
    tp::FixedFunction<std::string()> f([&s1]()
    {
        return s1;
    });
    auto f1 = std::move(f);
    ASSERT_EQ(s1, f1());
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}


