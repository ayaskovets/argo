#include <sandbox/net/unix/dgram/socket.hpp>
#include <sandbox/net/unix/sockaddr.hpp>

#include <benchmark/benchmark.h>

#include "templates/net.hpp"
#include "templates/templates.hpp"

namespace benchmarks::net::unix::dgram {

namespace unix = sandbox::net::unix;

BENCHMARK_T(net_unix_dgram, dgram_throughput, unix::dgram::socket,
            unix::sockaddr("BM_net_unix_dgram_dgram_throughput"))
    ->Threads(2)
    ->Args({1048576, 512})
    ->Args({1048576, 128})
    ->Unit(benchmark::TimeUnit::kMicrosecond);

}  // namespace benchmarks::net::unix::dgram
