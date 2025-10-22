#include <sandbox/net/inet/sockaddr.hpp>
#include <sandbox/net/inet/udp/socket.hpp>

#include <benchmark/benchmark.h>

namespace benchmarks::net::inet::udp {

void BM_net_inet_udp_throughput(benchmark::State& state) {
  static std::atomic_bool sender_done;

  const std::size_t data_size = state.range(0);
  const std::size_t buffer_size = state.range(1);

  const sandbox::net::inet::sockaddr sockaddr(
      sandbox::net::inet::ip::kLoopback(), sandbox::net::inet::port(8000));

  sandbox::net::inet::udp::socket socket;
  socket.set_nonblock(true);
  socket.set_reuseaddr(true);
  socket.set_reuseport(true);

  std::vector<std::byte> buffer(buffer_size);
  if (state.thread_index() == 0) {
    socket.bind(sockaddr);

    std::size_t loss = 0;
    for (const auto _ : state) {
      std::size_t received = 0;
      while (received < data_size) {
        const std::size_t _received = socket.receive(buffer);
        if (_received == 0 && sender_done.load(std::memory_order::relaxed)) {
          break;
        }
        received += _received;
      }
      loss += (data_size - received);
    }
    state.counters["loss"] = benchmark::Counter(
        loss * state.threads(), benchmark::Counter::kAvgIterations);
  } else {
    while (socket.connect(sockaddr) != sandbox::net::connect_status::kSuccess);

    for (const auto _ : state) {
      sender_done = false;
      std::size_t sent = 0;
      while (sent < data_size) {
        sent += socket.send(buffer);
      }
      sender_done = true;
    }
  }
}
BENCHMARK(BM_net_inet_udp_throughput)
    ->Threads(2)
    ->Args({1048576, 512})
    ->Args({1048576, 128})
    ->Unit(benchmark::TimeUnit::kMicrosecond);

}  // namespace benchmarks::net::inet::udp
