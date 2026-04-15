cmake_minimum_required(VERSION 3.25.0)
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
    templates/net.hpp
    templates/templates.hpp
    main.cpp)

# ------------------------------------------------------------------------------
# target
# ------------------------------------------------------------------------------

target_sources(wip-benchmarks PRIVATE ${SOURCES})

target_link_libraries(wip-benchmarks PRIVATE wip::wip benchmark::benchmark)

target_include_directories(wip-benchmarks PRIVATE ${CMAKE_CURRENT_LIST_DIR})

# ------------------------------------------------------------------------------
# workflow automation
# ------------------------------------------------------------------------------

add_test(NAME flow-benchmarks COMMAND flow-benchmarks --benchmark_color=true)
set_tests_properties(flow-benchmarks PROPERTIES LABELS benchmark)
