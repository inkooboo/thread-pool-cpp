thread-pool-cpp11
=================

1. *fast :P*.
2. C++11 compatible.
3. Header-only.

You need C++11 to compile this code.
Project file for Qt 5.1 provided.

Post job to thread pool is 10 times faster than for boost::asio based thread pool.

    *******begin tests*******
    ***thread_pool_t***
    Repost test [ENTER]
    reposted 1000001 in 343.757 ms
    reposted 1000001 in 343.757 ms
    reposted 1000001 in 343.757 ms
    reposted 1000001 in 343.757 ms

    ***asio_thread_pool_t***
    Repost test [ENTER]
    reposted 1000001 in 3500.07 ms
    reposted 1000001 in 3500.07 ms
    reposted 1000001 in 3500.07 ms
    reposted 1000001 in 3500.07 ms

See main.cpp for benchmark code.

