thread-pool-cpp11
=================

C++11 compatible *fast :P* thread pool based on lock-free queue.

Post job to thread pool is 4-5 times faster than for boost::asio based thread pool.

    *******begin tests*******
    ***thread_pool_t***
    Repost test [ENTER]
    reposted 100001 in 125.002 ms
    reposted 100001 in 125.002 ms
    reposted 100001 in 125.002 ms
    reposted 100001 in 125.002 ms

    ***asio_thread_pool_t***
    Repost test [ENTER]
    reposted 100001 in 593.761 ms
    reposted 100001 in 593.761 ms
    reposted 100001 in 578.136 ms
    reposted 100001 in 578.136 ms

See main.cpp for benchmark code.

You need C++11 to compile this code.
Project file for Qt 5 provided.

