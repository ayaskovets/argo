#pragma once

#include <wip/net/base_socket.hpp>

#include <atomic>

#include <benchmark/benchmark.h>

namespace benchmarks::net::templates {

namespace net = wip::net;
namespace io = wip::io;

template <typename Socket>
constexpr void nonblock_handshake(benchmark::State& state,
                                  const auto& sockaddr) noexcept {
  for (const auto _ : state) {
    Socket server;
    server.set_flags(io::fd::flag::kNonblock);
    server.set_reuseaddr(true);
    server.set_reuseport(true);
    server.bind(sockaddr);
    server.listen(1);

    Socket client;
    client.set_flags(io::fd::flag::kNonblock);
    client.set_reuseaddr(true);
    client.set_reuseport(true);

    while (client.connect(sockaddr) == net::connect_status::kRefused);
    Socket peer(io::uninitialized_t{});
    while (server.accept(peer) != net::accept_status::kSuccess);
  }
}

template <typename Socket>
constexpr void stream_throughput(benchmark::State& state,
                                 const auto& sockaddr) noexcept {
  const std::size_t data_size = state.range(0);
  const std::size_t buffer_size = state.range(1);

  Socket socket;
  socket.set_flags(io::fd::flag::kNonblock);
  socket.set_reuseaddr(true);
  socket.set_reuseport(true);

  std::vector<std::byte> buffer(buffer_size);
  if (state.thread_index() == 0) {
    socket.bind(sockaddr);
    socket.listen(1);

    Socket peer(io::uninitialized_t{});
    while (socket.accept(peer) != net::accept_status::kSuccess);

    for (const auto _ : state) {
      std::size_t received = 0;
      while (received < data_size) {
        received += peer.receive(buffer);
      }
    }
  } else {
    while (socket.connect(sockaddr) != net::connect_status::kSuccess);

    for (const auto _ : state) {
      std::size_t sent = 0;
      while (sent < data_size) {
        sent += socket.send(buffer);
      }
    }
  }
}

template <typename Socket>
constexpr void dgram_throughput(benchmark::State& state,
                                const auto& sockaddr) noexcept {
  static std::atomic_bool sender_done;

  const std::size_t data_size = state.range(0);
  const std::size_t buffer_size = state.range(1);

  Socket socket;
  socket.set_flags(io::fd::flag::kNonblock);
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
        if (_received == 0 && sender_done.load(std::memory_order::acquire)) {
          break;
        }
        received += _received;
      }
      loss += (data_size - received);
    }
    state.counters["loss"] = benchmark::Counter(
        loss * state.threads(), benchmark::Counter::kAvgIterations);
  } else {
    while (socket.connect(sockaddr) != net::connect_status::kSuccess);

    for (const auto _ : state) {
      sender_done.store(false, std::memory_order::release);
      std::size_t sent = 0;
      while (sent < data_size) {
        sent += socket.send(buffer);
      }
      sender_done.store(true, std::memory_order::release);
    }
  }

  sender_done.store(false, std::memory_order::release);
}

}  // namespace benchmarks::net::templates
