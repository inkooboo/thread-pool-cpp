#include <thread_pool.hpp>
#include <test.hpp>

#include <thread>
#include <future>
#include <functional>

int main() {
    std::cout << "*** Testing ThreadPool ***" << std::endl;

    doTest("post job", []() {
        ThreadPool pool;

        std::packaged_task<int()> t([](){
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            return 42;
        });

        std::future<int> r = t.get_future();

        pool.post(t);

        ASSERT(42 == r.get());
    });

    doTest("process job", []() {
        ThreadPool pool;

        std::future<int> r = pool.process([]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            return 42;
        });

        ASSERT(42 == r.get());
    });

    struct my_exception {};

    doTest("process job with exception", []() {
        ThreadPool pool;

        std::future<int> r = pool.process([]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            throw my_exception();
            return 42;
        });

        try {
            ASSERT(r.get() == 42 && !"should not be called, exception expected");
        } catch (const my_exception &e) {
        }
    });

}
