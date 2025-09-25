#include <benchmark/benchmark.h>

#include "net/unix/dgram/socket.hpp"
#include "net/unix/sockaddr.hpp"

namespace benchmarks::net::unix::dgram {

void BM_net_unix_dgram_throughput(benchmark::State& state) {
  static std::atomic_bool sender_done;

  const std::size_t data_size = state.range(0);
  const std::size_t buffer_size = state.range(1);

  const core::net::unix::sockaddr sockaddr(
      "BM_net_unix_dgram_throughput_sockaddr");

  core::net::unix::dgram::socket socket;
  socket.set_nonblock(true);

  std::vector<std::byte> buffer(buffer_size);
  if (state.thread_index() == 0) {
    socket.unlink_bind(sockaddr);

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
    while (socket.connect(sockaddr) != core::net::connect_status::kSuccess);

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
BENCHMARK(BM_net_unix_dgram_throughput)
    ->Threads(2)
    ->Args({1048576, 512})
    ->Args({1048576, 128})
    ->Unit(benchmark::TimeUnit::kMicrosecond);

}  // namespace benchmarks::net::unix::dgram
