thread-pool-cpp11
=================

C++11 compatible *fast :P* thread pool based on lock-free queue.

Post job to thread pool is 5-6 times faster than for boost::asio based thread pool.

    *******begin tests*******
    ***thread_pool_t***
    Repost test [ENTER]
    reposted 1000001 in 1125.02 ms
    reposted 1000001 in 1140.65 ms
    reposted 1000001 in 1140.65 ms
    reposted 1000001 in 1140.65 ms

    ***asio_thread_pool_t***
    Repost test [ENTER]
    reposted 1000001 in 6046.99 ms
    reposted 1000001 in 6062.62 ms
    reposted 1000001 in 6062.62 ms
    reposted 1000001 in 6078.24 ms

See main.cpp for benchmark code.

You need C++11 to compile this code.
Project file for Qt 5 provided.

