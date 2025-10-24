#include <sandbox/net/unix/sockaddr.hpp>
#include <sandbox/net/unix/stream/socket.hpp>

#include <benchmark/benchmark.h>

#include "templates/net.hpp"
#include "templates/templates.hpp"

namespace benchmarks::net::unix::stream {

namespace unix = sandbox::net::unix;

BENCHMARK_T(net_unix_stream, nonblock_handshake, unix::stream::socket,
            unix::sockaddr("BM_net_unix_stream_nonblock_handshake"))
    ->Unit(benchmark::TimeUnit::kMicrosecond);

BENCHMARK_T(net_unix_stream, stream_throughput, unix::stream::socket,
            unix::sockaddr("BM_net_unix_stream_throughput"))
    ->Threads(2)
    ->Args({1048576, 512})
    ->Args({1048576, 128})
    ->Unit(benchmark::TimeUnit::kMicrosecond);

}  // namespace benchmarks::net::unix::stream
