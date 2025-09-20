#include "net/inet6/udp/socket.hpp"

#include <gtest/gtest.h>

#include <thread>

#include "net/inet6/sockaddr.hpp"

namespace tests::net::inet6::udp {

TEST(net_inet6_udp_socket, size) {
  static_assert(sizeof(core::net::inet6::udp::socket) == 4);
  static_assert(alignof(core::net::inet6::udp::socket) == 4);
}

TEST(net_inet6_udp_socket, nonblocking_send_receive_error) {
  core::net::inet6::udp::socket socket;
  socket.set_nonblock(true);
  socket.set_reuseaddr(true);
  socket.set_reuseport(true);

  std::vector<std::byte> bytes;
  EXPECT_ANY_THROW(socket.send(bytes));
  EXPECT_EQ(socket.receive(bytes), 0);
}

TEST(net_inet6_udp_socket, bind) {
  const core::net::inet6::sockaddr sockaddr(core::net::inet6::ip::kLoopback(),
                                            core::net::inet6::port(9999));

  core::net::inet6::udp::socket socket;
  socket.set_reuseaddr(true);
  socket.set_reuseport(true);
  socket.bind(sockaddr);
  EXPECT_ANY_THROW(socket.bind(sockaddr));
}

TEST(net_inet6_udp_socket, connect) {
  const core::net::inet6::sockaddr sockaddr(core::net::inet6::ip::kLoopback(),
                                            core::net::inet6::port(9998));

  core::net::inet6::udp::socket socket;
  socket.set_reuseaddr(true);
  socket.set_reuseport(true);

  EXPECT_EQ(socket.connect(sockaddr),
            core::net::inet6::udp::socket::connect_status::kSuccess);
  EXPECT_EQ(socket.connect(sockaddr),
            core::net::inet6::udp::socket::connect_status::kSuccess);
}

TEST(net_inet6_udp_socket, sockaddrs) {
  const core::net::inet6::sockaddr bind_sockaddr(
      core::net::inet6::ip::kLoopback(), core::net::inet6::port(9997));
  const core::net::inet6::sockaddr connect_sockaddr(
      core::net::inet6::ip::kLoopback(), core::net::inet6::port(9996));

  core::net::inet6::udp::socket socket;
  socket.set_reuseaddr(true);
  socket.set_reuseport(true);

  core::net::inet6::sockaddr out_sockaddr(core::net::inet6::ip::kAny(),
                                          core::net::inet6::port(0));
  EXPECT_NO_THROW(socket.get_bind_sockaddr(out_sockaddr));
  EXPECT_NE((socket.get_bind_sockaddr(out_sockaddr), out_sockaddr),
            bind_sockaddr);
  socket.bind(bind_sockaddr);
  EXPECT_EQ((socket.get_bind_sockaddr(out_sockaddr), out_sockaddr),
            bind_sockaddr);
  EXPECT_ANY_THROW(socket.get_connect_sockaddr(out_sockaddr));

  EXPECT_EQ(socket.connect(connect_sockaddr),
            core::net::inet6::udp::socket::connect_status::kSuccess);
  EXPECT_EQ((socket.get_connect_sockaddr(out_sockaddr), out_sockaddr),
            connect_sockaddr);
}

TEST(net_inet6_udp_socket, blocking_echo_handshake) {
  const std::vector<std::byte> kBuffer{std::byte(1), std::byte(2),
                                       std::byte(3)};

  const core::net::inet6::sockaddr server_sockaddr(
      core::net::inet6::ip::kLoopback(), core::net::inet6::port(9995));
  const core::net::inet6::sockaddr client_sockaddr(
      core::net::inet6::ip::kLoopback(), core::net::inet6::port(9996));

  core::net::inet6::udp::socket server;
  server.set_reuseaddr(true);
  server.set_reuseport(true);
  server.bind(server_sockaddr);

  core::net::inet6::udp::socket client;
  client.set_reuseaddr(true);
  client.set_reuseport(true);
  client.bind(client_sockaddr);

  std::jthread server_thread([&server, &client_sockaddr, &kBuffer] {
    core::net::inet6::sockaddr peer(core::net::inet6::ip::kAny(),
                                    core::net::inet6::port(0));
    std::vector<std::byte> buffer(kBuffer.size());
    EXPECT_EQ(server.receive_from(buffer, peer), kBuffer.size());
    EXPECT_EQ(buffer, kBuffer);
    EXPECT_EQ(peer, client_sockaddr);
    EXPECT_EQ(server.send_to(kBuffer, peer), kBuffer.size());

    EXPECT_EQ(server.receive(buffer), kBuffer.size());
    EXPECT_EQ(server.connect(client_sockaddr),
              core::net::inet6::udp::socket::connect_status::kSuccess);
    EXPECT_EQ(server.send(kBuffer), kBuffer.size());
  });

  core::net::inet6::sockaddr peer(core::net::inet6::ip::kAny(),
                                  core::net::inet6::port(0));
  std::vector<std::byte> buffer(kBuffer.size());
  EXPECT_EQ(client.send_to(kBuffer, server_sockaddr), kBuffer.size());
  EXPECT_EQ(client.receive_from(buffer, peer), kBuffer.size());
  EXPECT_EQ(buffer, kBuffer);
  EXPECT_EQ(peer, server_sockaddr);
  EXPECT_EQ(client.connect(server_sockaddr),
            core::net::inet6::udp::socket::connect_status::kSuccess);
  EXPECT_EQ(client.send(kBuffer), kBuffer.size());
  EXPECT_EQ(client.receive(buffer), kBuffer.size());
}

TEST(net_inet6_udp_socket, nonblocking_echo_handshake) {
  const std::vector<std::byte> kBuffer{std::byte(1), std::byte(2),
                                       std::byte(3)};

  const core::net::inet6::sockaddr server_sockaddr(
      core::net::inet6::ip::kLoopback(), core::net::inet6::port(9995));
  const core::net::inet6::sockaddr client_sockaddr(
      core::net::inet6::ip::kLoopback(), core::net::inet6::port(9996));

  core::net::inet6::udp::socket server;
  server.set_nonblock(true);
  server.set_reuseaddr(true);
  server.set_reuseport(true);
  server.bind(server_sockaddr);

  core::net::inet6::udp::socket client;
  client.set_nonblock(true);
  client.set_reuseaddr(true);
  client.set_reuseport(true);
  client.bind(client_sockaddr);

  std::vector<std::byte> buffer(kBuffer.size());
  core::net::inet6::sockaddr peer(core::net::inet6::ip::kNonRoutable(),
                                  core::net::inet6::port(0));
  EXPECT_EQ(server.send_to(kBuffer, client_sockaddr), kBuffer.size());
  while (client.receive_from(buffer, peer) != kBuffer.size());
  EXPECT_EQ(buffer, kBuffer);
  EXPECT_EQ(peer, server_sockaddr);

  server.connect(client_sockaddr);
  client.connect(server_sockaddr);

  EXPECT_EQ(client.send(kBuffer), kBuffer.size());
  while (server.receive(buffer) != kBuffer.size());
  EXPECT_EQ(buffer, kBuffer);
}

}  // namespace tests::net::inet6::udp
