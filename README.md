thread-pool-cpp
=================

 * It is highly scalable and fast.
 * It is header only.
 * It implements both work-stealing and work-distribution balancing startegies.
 * It implements cooperative scheduling strategy for tasks.

Example run:
Post job to thread pool is much faster than for boost::asio based thread pool.

    ***thread pool cpp11***
    Repost test [ENTER]
    reposted 1000001 in 61.6419 ms
    reposted 1000001 in 65.1405 ms
    reposted 1000001 in 66.2461 ms
    reposted 1000001 in 70.908 ms

    ***asio thread pool***
    Repost test [ENTER]
    reposted 1000001 in 1410.18 ms
    reposted 1000001 in 1415.82 ms
    reposted 1000001 in 1416.03 ms
    reposted 1000001 in 1418.27 ms

See benchmark/main.cpp for benchmark code.

