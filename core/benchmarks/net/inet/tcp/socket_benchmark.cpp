#include <benchmark/benchmark.h>

#include "net/inet/sockaddr.hpp"
#include "net/inet/tcp/socket.hpp"

namespace benchmarks::net::inet::tcp {

void BM_net_inet_tcp_nonblock_handshake(benchmark::State& state) {
  const core::net::inet::sockaddr sockaddr(core::net::inet::ip::kLoopback(),
                                           core::net::inet::port(8000));

  for (const auto _ : state) {
    core::net::inet::tcp::socket server;
    server.set_nonblock(true);
    server.set_reuseaddr(true);
    server.set_reuseport(true);
    server.bind(sockaddr);
    server.listen(1);

    core::net::inet::tcp::socket client;
    client.set_nonblock(true);
    client.set_reuseaddr(true);
    client.set_reuseport(true);

    while (client.connect(sockaddr) !=
           core::net::inet::tcp::socket::connect_status::kPending);
    core::net::inet::tcp::socket peer(core::io::fd::uninitialized_t{});
    while (server.accept(peer) !=
           core::net::inet::tcp::socket::accept_status::kSuccess);
  }
}
BENCHMARK(BM_net_inet_tcp_nonblock_handshake)
    ->Unit(benchmark::TimeUnit::kMicrosecond);

void BM_net_inet_tcp_throughput(benchmark::State& state) {
  const std::size_t data_size = state.range(0);
  const std::size_t buffer_size = state.range(1);

  const core::net::inet::sockaddr sockaddr(core::net::inet::ip::kLoopback(),
                                           core::net::inet::port(8000));

  core::net::inet::tcp::socket socket;
  socket.set_nonblock(true);
  socket.set_reuseaddr(true);
  socket.set_reuseport(true);

  std::vector<std::byte> buffer(buffer_size);
  if (state.thread_index() == 0) {
    socket.bind(sockaddr);
    socket.listen(1);

    core::net::inet::tcp::socket peer(core::io::fd::uninitialized_t{});
    while (socket.accept(peer) !=
           core::net::inet::tcp::socket::accept_status::kSuccess);

    for (const auto _ : state) {
      std::size_t received = 0;
      while (received < data_size) {
        received += peer.receive(buffer);
      }
    }
  } else {
    while (socket.connect(sockaddr) !=
           core::net::inet::tcp::socket::connect_status::kSuccess);

    for (const auto _ : state) {
      std::size_t sent = 0;
      while (sent < data_size) {
        sent += socket.send(buffer);
      }
    }
  }
}
BENCHMARK(BM_net_inet_tcp_throughput)
    ->Threads(2)
    ->Args({1048576, 512})
    ->Args({1048576, 128})
    ->Unit(benchmark::TimeUnit::kMicrosecond);

}  // namespace benchmarks::net::inet::tcp
