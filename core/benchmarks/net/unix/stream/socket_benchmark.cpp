#include <benchmark/benchmark.h>

#include "net/unix/sockaddr.hpp"
#include "net/unix/stream/socket.hpp"

namespace benchmarks::net::unix::stream {

void BM_net_unix_stream_nonblock_handshake(benchmark::State& state) {
  const core::net::unix::sockaddr sockaddr(
      "BM_net_unix_stream_nonblock_handshake");

  for (const auto _ : state) {
    core::net::unix::stream::socket server;
    server.set_nonblock(true);
    server.set_reuseaddr(true);
    server.set_reuseport(true);
    server.unlink_bind(sockaddr);
    server.listen(1);

    core::net::unix::stream::socket client;
    client.set_nonblock(true);
    client.set_reuseaddr(true);
    client.set_reuseport(true);

    while (client.connect(sockaddr) == core::net::connect_status::kRefused);
    core::net::unix::stream::socket peer(core::io::fd::uninitialized_t{});
    while (server.accept(peer) != core::net::accept_status::kSuccess);
  }
}
BENCHMARK(BM_net_unix_stream_nonblock_handshake)
    ->Unit(benchmark::TimeUnit::kMicrosecond);

void BM_net_unix_stream_throughput(benchmark::State& state) {
  const std::size_t data_size = state.range(0);
  const std::size_t buffer_size = state.range(1);

  const core::net::unix::sockaddr sockaddr(
      "BM_net_unix_stream_throughput_sockaddr");

  core::net::unix::stream::socket socket;
  socket.set_nonblock(true);

  std::vector<std::byte> buffer(buffer_size);
  if (state.thread_index() == 0) {
    socket.unlink_bind(sockaddr);
    socket.listen(1);

    core::net::unix::stream::socket peer(core::io::fd::uninitialized_t{});
    while (socket.accept(peer) != core::net::accept_status::kSuccess);

    for (const auto _ : state) {
      std::size_t received = 0;
      while (received < data_size) {
        received += peer.receive(buffer);
      }
    }
  } else {
    while (socket.connect(sockaddr) != core::net::connect_status::kSuccess);

    for (const auto _ : state) {
      std::size_t sent = 0;
      while (sent < data_size) {
        sent += socket.send(buffer);
      }
    }
  }
}
BENCHMARK(BM_net_unix_stream_throughput)
    ->Threads(2)
    ->Args({1048576, 512})
    ->Args({1048576, 128})
    ->Unit(benchmark::TimeUnit::kMicrosecond);

}  // namespace benchmarks::net::unix::stream
