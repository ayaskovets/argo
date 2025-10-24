#include <sandbox/net/inet/sockaddr.hpp>
#include <sandbox/net/inet/tcp/socket.hpp>

#include <benchmark/benchmark.h>

#include "templates/net.hpp"
#include "templates/templates.hpp"

namespace benchmarks::net::inet::tcp {

namespace inet = sandbox::net::inet;

BENCHMARK_T(net_inet_tcp, nonblock_handshake, inet::tcp::socket,
            inet::sockaddr(inet::ip::kLoopback(), inet::port(8000)))
    ->Unit(benchmark::TimeUnit::kMicrosecond);

BENCHMARK_T(net_inet_tcp, stream_throughput, inet::tcp::socket,
            inet::sockaddr(inet::ip::kLoopback(), inet::port(8000)))
    ->Threads(2)
    ->Args({1048576, 512})
    ->Args({1048576, 128})
    ->Unit(benchmark::TimeUnit::kMicrosecond);

}  // namespace benchmarks::net::inet::tcp
