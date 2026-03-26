#include <wip/net/inet/sockaddr.hpp>
#include <wip/net/inet/udp/socket.hpp>

#include <benchmark/benchmark.h>

#include "templates/net.hpp"
#include "templates/templates.hpp"

namespace benchmarks::net::inet::udp {

namespace inet = wip::net::inet;

BENCHMARK_T(net_inet_udp, dgram_throughput, inet::udp::socket,
            inet::sockaddr(inet::ip::kLoopback(), inet::port(8000)))
    ->Threads(2)
    ->Args({1048576, 512})
    ->Args({1048576, 128})
    ->Unit(benchmark::TimeUnit::kMicrosecond);

}  // namespace benchmarks::net::inet::udp
