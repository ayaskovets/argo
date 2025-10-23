set(TARGET benchmarks)

find_package(benchmark REQUIRED)

add_executable(${TARGET})
target_sources(${TARGET}
    PRIVATE
    datetime/iso8601_benchmark.cpp
    logging/logger_benchmark.cpp
    net/inet/sockaddr_benchmark.cpp
    net/inet/tcp/socket_benchmark.cpp
    net/inet/udp/socket_benchmark.cpp
    net/inet6/sockaddr_benchmark.cpp
    net/inet6/tcp/socket_benchmark.cpp
    net/inet6/udp/socket_benchmark.cpp
    net/resolve_benchmark.cpp
    net/unix/dgram/socket_benchmark.cpp
    net/unix/sockaddr_benchmark.cpp
    net/unix/stream/socket_benchmark.cpp
    queues/locked_mpmc_queue_benchmark.cpp
    queues/locked_std_deque_benchmark.cpp
    queues/lockfree_mpmc_queue_benchmark.cpp
    queues/lockfree_mpsc_queue_benchmark.cpp
    queues/lockfree_spmc_queue_benchmark.cpp
    queues/lockless_mpmc_queue_benchmark.cpp
    queues/single_threaded_queue_benchmark.cpp
    queues/waitfree_spsc_queue_benchmark.cpp
    main.cpp
)
target_compile_features(${TARGET}
    PRIVATE cxx_std_23
)
target_include_directories(${TARGET}
    PUBLIC benchmark templates
)
target_link_libraries(${TARGET}
    PRIVATE sandbox benchmark::benchmark
)
