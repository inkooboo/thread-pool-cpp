thread-pool-cpp11
=================

1. *fast :P*.
2. C++11 compatible.
3. Header-only.
4. Work stealing pattern.

You need C++11 to compile this code.
Project file for Qt 5.1 provided.

Post job to thread pool is up to 10 times faster than for boost::asio based thread pool.

    *******begin tests*******
    ***thread_pool_t***
    Repost test [ENTER]
    reposted 1000001 in 406.25 ms
    reposted 1000001 in 406.25 ms
    reposted 1000001 in 406.25 ms
    reposted 1000001 in 406.25 ms

    ***asio_thread_pool_t***
    Repost test [ENTER]
    reposted 1000001 in 3906.25 ms
    reposted 1000001 in 3921.88 ms
    reposted 1000001 in 3921.88 ms
    reposted 1000001 in 3921.88 ms

See main.cpp for benchmark code.

