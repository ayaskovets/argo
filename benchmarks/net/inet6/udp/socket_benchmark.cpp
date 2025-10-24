#include <sandbox/net/inet6/sockaddr.hpp>
#include <sandbox/net/inet6/udp/socket.hpp>

#include <benchmark/benchmark.h>

#include "templates/net.hpp"
#include "templates/templates.hpp"

namespace benchmarks::net::inet6::udp {

namespace inet6 = sandbox::net::inet6;

BENCHMARK_T(net_inet6_udp, dgram_throughput, inet6::udp::socket,
            inet6::sockaddr(inet6::ip::kLoopback(), inet6::port(8000)))
    ->Threads(2)
    ->Args({1048576, 512})
    ->Args({1048576, 128})
    ->Unit(benchmark::TimeUnit::kMicrosecond);

}  // namespace benchmarks::net::inet6::udp
