#include <thread_pool/thread_pool.hpp>
#include <test.hpp>

#include <thread>
#include <future>
#include <functional>
#include <memory>

using namespace tp;

using ThreadPoolStd = ThreadPool<>;

int main()
{
    std::cout << "*** Testing TP ***" << std::endl;

    doTest("post job", []()
        {
            ThreadPoolStd pool;

            std::packaged_task<int()> t([]()
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    return 42;
                });

            std::future<int> r = t.get_future();

            pool.post(t);

            ASSERT(42 == r.get());
        });

        try {
            ASSERT(r.get() == 42 && !"should not be called, exception expected");
        } catch (const my_exception &e) {
        }
    });

    doTest("post job to threadpool with onStart/onStop", []() {
        std::atomic<int> someValue{0};
        ThreadPoolOptions options;
        options.onStart = [&someValue](){ ++someValue; };
        options.onStop = [&someValue](){ --someValue; };

        if (true) {
            ThreadPool pool{options};

            std::packaged_task<int()> t([&someValue](){
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                return someValue.load();
            });

            std::future<int> r = t.get_future();

            pool.post(t);

            const auto result = r.get();

            ASSERT(0 < result);
            ASSERT(pool.getWorkerCount() == result);
        }

        ASSERT(0 == someValue);
    });

}
