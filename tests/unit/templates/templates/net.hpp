#pragma once

#pragma once

#include <sandbox/net/base_socket.hpp>

#include <thread>

#include <gtest/gtest.h>

namespace tests::net::templates {

namespace io = sandbox::io;
namespace net = sandbox::net;

template <typename Socket>
constexpr void stream_unconnected_send_receive() noexcept {
  const std::vector<std::byte> data{std::byte(1), std::byte(2), std::byte(3)};

  Socket socket;
  socket.set_reuseaddr(true);
  socket.set_reuseport(true);

  EXPECT_ANY_THROW(socket.send(data));
  std::vector<std::byte> buffer(data.size());
  EXPECT_ANY_THROW(socket.receive(buffer));
}

template <typename Socket>
constexpr void dgram_unconnected_send_receive(const auto& sockaddr) noexcept {
  const std::vector<std::byte> data{std::byte(1), std::byte(2), std::byte(3)};

  Socket socket;
  socket.set_flags(io::fd::flag::kNonblock);
  socket.set_reuseaddr(true);
  socket.set_reuseport(true);

  EXPECT_EQ(socket.send_to(data, sockaddr), data.size());
  std::vector<std::byte> buffer(data.size());
  EXPECT_EQ(socket.receive(buffer), 0);
}

template <typename Socket>
constexpr void bind(const auto& sockaddr) noexcept {
  Socket socket;
  socket.set_reuseaddr(true);
  socket.set_reuseport(true);
  EXPECT_EQ(socket.bind(sockaddr), net::bind_status::kSuccess);
  EXPECT_ANY_THROW(socket.bind(sockaddr));

  Socket other;
  EXPECT_EQ(other.bind(sockaddr), net::bind_status::kInUse);
  other.set_reuseaddr(true);
  other.set_reuseport(true);
  EXPECT_EQ(other.bind(sockaddr), net::bind_status::kSuccess);
}

template <typename Socket>
constexpr void connect(const auto& sockaddr,
                       const auto& connect_status) noexcept {
  Socket socket;
  socket.set_reuseaddr(true);
  socket.set_reuseport(true);

  EXPECT_EQ(socket.connect(sockaddr), connect_status);
}

template <typename Socket>
constexpr void sockaddrs(const auto& bind_sockaddr,
                         const auto& connect_sockaddr,
                         const auto& connect_status) noexcept {
  Socket socket;
  socket.set_flags(io::fd::flag::kNonblock);
  socket.set_reuseaddr(true);
  socket.set_reuseport(true);

  auto out_sockaddr = connect_sockaddr;
  EXPECT_NO_THROW((socket.get_bind_sockaddr(out_sockaddr)));
  EXPECT_NE((socket.get_bind_sockaddr(out_sockaddr), out_sockaddr),
            bind_sockaddr);
  socket.bind(bind_sockaddr);
  EXPECT_EQ((socket.get_bind_sockaddr(out_sockaddr), out_sockaddr),
            bind_sockaddr);
  EXPECT_ANY_THROW(socket.get_connect_sockaddr(out_sockaddr));

  out_sockaddr = bind_sockaddr;
  EXPECT_NE(out_sockaddr, connect_sockaddr);
  EXPECT_EQ(socket.connect(connect_sockaddr), connect_status);
  if (connect_status == net::connect_status::kSuccess) {
    EXPECT_EQ((socket.get_connect_sockaddr(out_sockaddr), out_sockaddr),
              connect_sockaddr);
  } else {
    EXPECT_ANY_THROW(socket.get_connect_sockaddr(out_sockaddr));
  }
}

template <typename Socket>
constexpr void unconnected_accept(const auto& sockaddr) noexcept {
  Socket socket;
  socket.set_flags(io::fd::flag::kNonblock);
  socket.set_reuseaddr(true);
  socket.set_reuseport(true);
  socket.bind(sockaddr);

  Socket peer(io::uninitialized_t{});
  EXPECT_ANY_THROW(socket.accept(peer));
  socket.listen(1);
  EXPECT_EQ(socket.accept(peer), net::accept_status::kEmptyQueue);
}

template <typename Socket>
constexpr void stream_blocking_handshake(const auto& sockaddr) noexcept {
  const std::vector<std::byte> data{std::byte(1), std::byte(2), std::byte(3)};

  Socket server;
  server.set_reuseaddr(true);
  server.set_reuseport(true);
  server.bind(sockaddr);
  server.listen(1);

  const std::jthread server_thread([&server, &data] {
    Socket peer;
    EXPECT_EQ(server.accept(peer), net::accept_status::kSuccess);

    std::vector<std::byte> buffer(data.size());
    EXPECT_EQ(peer.receive(buffer), data.size());
    EXPECT_EQ(buffer, data);
    EXPECT_EQ(peer.send(data), data.size());
  });

  Socket client;
  client.set_reuseaddr(true);
  client.set_reuseport(true);

  EXPECT_EQ(client.connect(sockaddr), net::connect_status::kSuccess);
  EXPECT_EQ(client.send(data), data.size());
  std::vector<std::byte> buffer(data.size());
  EXPECT_EQ(client.receive(buffer), data.size());
  EXPECT_EQ(buffer, data);
}

template <typename Socket>
constexpr void dgram_blocking_handshake(const auto& server_sockaddr,
                                        const auto& client_sockaddr) noexcept {
  const std::vector<std::byte> data{std::byte(1), std::byte(2), std::byte(3)};

  Socket server;
  server.set_reuseaddr(true);
  server.set_reuseport(true);
  server.bind(server_sockaddr);

  const std::jthread server_thread([&server, &client_sockaddr, &data] {
    auto peer(client_sockaddr);
    std::vector<std::byte> buffer(data.size());
    EXPECT_EQ(server.receive_from(buffer, peer), data.size());
    EXPECT_EQ(buffer, data);
    EXPECT_EQ(peer, client_sockaddr);
    EXPECT_EQ(server.send_to(data, peer), data.size());

    EXPECT_EQ(server.receive(buffer), data.size());
    EXPECT_EQ(server.connect(client_sockaddr), net::connect_status::kSuccess);
    EXPECT_EQ(server.send(data), data.size());
  });

  auto peer(server_sockaddr);
  std::vector<std::byte> buffer(data.size());

  Socket client;
  client.set_reuseaddr(true);
  client.set_reuseport(true);
  client.bind(client_sockaddr);

  EXPECT_EQ(client.send_to(data, server_sockaddr), data.size());
  EXPECT_EQ(client.receive_from(buffer, peer), data.size());
  EXPECT_EQ(buffer, data);
  EXPECT_EQ(peer, server_sockaddr);

  EXPECT_EQ(client.connect(server_sockaddr), net::connect_status::kSuccess);
  EXPECT_EQ(client.send(data), data.size());
  EXPECT_EQ(client.receive(buffer), data.size());
  EXPECT_EQ(buffer, data);
}

template <typename Socket>
constexpr void stream_nonblocking_handshake(
    const auto& bind_sockaddr, const auto& connect_status) noexcept {
  const std::vector<std::byte> data{std::byte(1), std::byte(2), std::byte(3)};

  Socket server;
  server.set_flags(io::fd::flag::kNonblock);
  server.set_reuseaddr(true);
  server.set_reuseport(true);
  server.bind(bind_sockaddr);
  server.listen(1);

  Socket client;
  client.set_flags(io::fd::flag::kNonblock);
  client.set_reuseaddr(true);
  client.set_reuseport(true);

  EXPECT_EQ(client.connect(bind_sockaddr), connect_status);
  Socket peer(io::uninitialized_t{});
  while (server.accept(peer) != net::accept_status::kSuccess);

  EXPECT_EQ(peer.send(data), data.size());
  std::vector<std::byte> buffer(data.size());
  while (client.receive(buffer) != data.size());
  EXPECT_EQ(buffer, data);
}

template <typename Socket>
constexpr void dgram_nonblocking_handshake(
    const auto& server_sockaddr, const auto& client_sockaddr) noexcept {
  const std::vector<std::byte> data{std::byte(1), std::byte(2), std::byte(3)};

  Socket server;
  server.set_flags(io::fd::flag::kNonblock);
  server.set_reuseaddr(true);
  server.set_reuseport(true);
  server.bind(server_sockaddr);

  Socket client;
  client.set_flags(io::fd::flag::kNonblock);
  client.set_reuseaddr(true);
  client.set_reuseport(true);
  client.bind(client_sockaddr);

  auto peer(server_sockaddr);
  std::vector<std::byte> buffer(data.size());

  EXPECT_EQ(server.send_to(data, client_sockaddr), data.size());
  while (client.receive_from(buffer, peer) != data.size());
  EXPECT_EQ(buffer, data);
  EXPECT_EQ(peer, server_sockaddr);

  server.connect(client_sockaddr);
  client.connect(server_sockaddr);

  EXPECT_EQ(client.send(data), data.size());
  while (server.receive(buffer) != data.size());
  EXPECT_EQ(buffer, data);
}

}  // namespace tests::net::templates
