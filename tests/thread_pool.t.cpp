#include <thread_pool/thread_pool.hpp>
#include <test.hpp>

#include <future>
#include <functional>
#include <sstream>
#include <thread>
#include <tuple>

using namespace tp;

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

    doTest("multiple compilation units", []() {
        extern size_t getWorkerIdForCurrentThread();
        extern size_t getWorkerIdForCurrentThread2();

        ThreadPool pool;

        std::future<std::tuple<size_t, size_t, size_t, size_t>> r = pool.process([]() {
            return std::make_tuple(Worker::getWorkerIdForCurrentThread(),
                                   *detail::thread_id(),
                                   getWorkerIdForCurrentThread(),
                                   getWorkerIdForCurrentThread2());
        });

        const auto t = r.get();
        const auto id0 = std::get<0>(t);
        const auto id1 = std::get<1>(t);
        const auto id2 = std::get<2>(t);
        const auto id3 = std::get<3>(t);

        std::cout << " " << id0 << " " << id1 << " " << id2 << " " << id3;

        ASSERT(id0 == id1);
        ASSERT(id1 == id2);
        ASSERT(id2 == id3);
    });

}
