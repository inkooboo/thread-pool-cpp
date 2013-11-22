thread-pool-cpp11
=================

C++11 compatible *fast :P* thread pool.

Post job to thread pool is 5-6 times faster than for boost::asio based thread pool.

    *******begin tests*******
    ***thread_pool_t***
    Repost test [ENTER]
    reposted 1000001 in 687.513 ms
    reposted 1000001 in 687.513 ms
    reposted 1000001 in 734.389 ms
    reposted 1000001 in 734.389 ms

    ***asio_thread_pool_t***
    Repost test [ENTER]
    reposted 1000001 in 3609.44 ms
    reposted 1000001 in 3625.07 ms
    reposted 1000001 in 3625.07 ms
    reposted 1000001 in 3625.07 ms

See main.cpp for benchmark code.

You need C++11 to compile this code.
Project file for Qt 5 provided.

