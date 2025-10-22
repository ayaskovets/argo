#include <sandbox/net/inet6/sockaddr.hpp>
#include <sandbox/net/inet6/tcp/socket.hpp>

#include <benchmark/benchmark.h>

namespace benchmarks::net::inet6::tcp {

void BM_net_inet6_tcp_nonblock_handshake(benchmark::State& state) {
  const sandbox::net::inet6::sockaddr sockaddr(
      sandbox::net::inet6::ip::kLoopback(), sandbox::net::inet6::port(8000));

  for (const auto _ : state) {
    sandbox::net::inet6::tcp::socket server;
    server.set_nonblock(true);
    server.set_reuseaddr(true);
    server.set_reuseport(true);
    server.bind(sockaddr);
    server.listen(1);

    sandbox::net::inet6::tcp::socket client;
    client.set_nonblock(true);
    client.set_reuseaddr(true);
    client.set_reuseport(true);

    while (client.connect(sockaddr) != sandbox::net::connect_status::kPending);
    sandbox::net::inet6::tcp::socket peer(sandbox::io::uninitialized_t{});
    while (server.accept(peer) != sandbox::net::accept_status::kSuccess);
  }
}
BENCHMARK(BM_net_inet6_tcp_nonblock_handshake)
    ->Unit(benchmark::TimeUnit::kMicrosecond);

void BM_net_inet6_tcp_throughput(benchmark::State& state) {
  const std::size_t data_size = state.range(0);
  const std::size_t buffer_size = state.range(1);

  const sandbox::net::inet6::sockaddr sockaddr(
      sandbox::net::inet6::ip::kLoopback(), sandbox::net::inet6::port(8000));

  sandbox::net::inet6::tcp::socket socket;
  socket.set_nonblock(true);
  socket.set_reuseaddr(true);
  socket.set_reuseport(true);

  std::vector<std::byte> buffer(buffer_size);
  if (state.thread_index() == 0) {
    socket.bind(sockaddr);
    socket.listen(1);

    sandbox::net::inet6::tcp::socket peer(sandbox::io::uninitialized_t{});
    while (socket.accept(peer) != sandbox::net::accept_status::kSuccess);

    for (const auto _ : state) {
      std::size_t received = 0;
      while (received < data_size) {
        received += peer.receive(buffer);
      }
    }
  } else {
    while (socket.connect(sockaddr) != sandbox::net::connect_status::kSuccess);

    for (const auto _ : state) {
      std::size_t sent = 0;
      while (sent < data_size) {
        sent += socket.send(buffer);
      }
    }
  }
}
BENCHMARK(BM_net_inet6_tcp_throughput)
    ->Threads(2)
    ->Args({1048576, 512})
    ->Args({1048576, 128})
    ->Unit(benchmark::TimeUnit::kMicrosecond);

}  // namespace benchmarks::net::inet6::tcp
