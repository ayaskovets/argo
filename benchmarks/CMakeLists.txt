cmake_minimum_required(VERSION 3.23.0)
project(wip-benchmarks LANGUAGES CXX)

# ------------------------------------------------------------------------------
# dependencies
# ------------------------------------------------------------------------------

find_package(benchmark REQUIRED)

if(PROJECT_IS_TOP_LEVEL)
  find_package(wip REQUIRED)
endif()

include(CTest)
include(GoogleTest)

# ------------------------------------------------------------------------------
# general settings and options
# ------------------------------------------------------------------------------

add_executable(wip-benchmarks)

# ------------------------------------------------------------------------------
# sources
# ------------------------------------------------------------------------------

set(SOURCES
    datetime/iso8601_bench.cpp
    logging/logger_bench.cpp
    net/inet/sockaddr_bench.cpp
    net/inet/tcp/socket_bench.cpp
    net/inet/udp/socket_bench.cpp
    net/inet6/sockaddr_bench.cpp
    net/inet6/tcp/socket_bench.cpp
    net/inet6/udp/socket_bench.cpp
    net/resolve_bench.cpp
    net/unix/dgram/socket_bench.cpp
    net/unix/sockaddr_bench.cpp
    net/unix/stream/socket_bench.cpp
    queues/locked_mpmc_queue_bench.cpp
    queues/locked_std_deque_bench.cpp
    queues/lockfree_mpmc_queue_bench.cpp
    queues/lockfree_mpsc_queue_bench.cpp
    queues/lockfree_spmc_queue_bench.cpp
    queues/lockless_mpmc_queue_bench.cpp
    queues/single_threaded_queue_bench.cpp
    queues/waitfree_spsc_queue_bench.cpp
    main.cpp)

# ------------------------------------------------------------------------------
# target
# ------------------------------------------------------------------------------

target_sources(wip-benchmarks PRIVATE ${SOURCES})

target_link_libraries(wip-benchmarks PRIVATE wip::wip benchmark::benchmark)
