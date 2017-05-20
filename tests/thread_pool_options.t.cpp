#include <gtest/gtest.h>

#include <thread_pool/thread_pool_options.hpp>

#include <thread>

TEST(ThreadPoolOptions, ctor)
{
    tp::ThreadPoolOptions options;

    ASSERT_EQ(1024, options.queueSize());
    ASSERT_EQ(std::max<size_t>(1u, std::thread::hardware_concurrency()),
              options.threadCount());
}

TEST(ThreadPoolOptions, modification)
{
    tp::ThreadPoolOptions options;

    options.setThreadCount(5);
    ASSERT_EQ(5, options.threadCount());

    options.setQueueSize(32);
    ASSERT_EQ(32, options.queueSize());
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
