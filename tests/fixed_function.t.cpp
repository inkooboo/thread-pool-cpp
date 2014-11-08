#include <fixed_function.hpp>
#include <test.hpp>

#include <string>
#include <cassert>
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
    int b(const int &p) {
        return p;
    }
};

template <typename T>
struct Foo
{
    template <typename U>
    U bar(U p) {
        return p + payload;
    }

    T payload;
};

template <typename T>
void print_overhead() {
    using func_type = FixedFunction<void(), sizeof(T)>;
    int t_s = sizeof(T);
    int f_s = sizeof(func_type);
    std::cout << " - for type size " << t_s << "\n"
              << "    function size is " << f_s << "\n"
              << "    overhead is " << float(f_s - t_s)/t_s * 100 << "%\n";
}

int main()
{
    print_overhead<char[8]>();
    print_overhead<char[16]>();
    print_overhead<char[32]>();
    print_overhead<char[64]>();
    print_overhead<char[128]>();

    doTest("free func", []() {
        FixedFunction<int(int)> f(test_free_func);
        ASSERT(3 == f(3));
    });

    doTest("free func template", []() {
        FixedFunction<std::string(std::string)> f(test_free_func_template<std::string>);
        ASSERT(std::string("abc") == f("abc"));
    });


    doTest("void func", []() {
        FixedFunction<void(int &, int)> f(test_void);
        int p = 0;
        f(p, 42);
        ASSERT(42 == p);
    });

    doTest("class method 1", []() {
        using namespace std::placeholders;
        A a;
        FixedFunction<int(const int&)> f(std::bind(&A::b, &a, _1));
        ASSERT(4 == f(4));
    });

    doTest("class method 2", []() {
        using namespace std::placeholders;
        Foo<float> foo;
        foo.payload = 1.f;
        FixedFunction<int(int)> f(std::bind(&Foo<float>::bar<int>, &foo, _1));
        ASSERT(2 == f(1));
    });

    doTest("lambda", []() {
        const std::string s1 = "s1";
        FixedFunction<std::string()> f([&s1]() {
            return s1;
        });

        ASSERT(s1 == f());
    });

}



