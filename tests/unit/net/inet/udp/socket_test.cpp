#include <sandbox/net/inet/sockaddr.hpp>
#include <sandbox/net/inet/udp/socket.hpp>

#include <thread>

#include <gtest/gtest.h>

namespace tests::net::inet::udp {

namespace inet = sandbox::net::inet;

TEST(net_inet_udp_socket, size) {
  static_assert(sizeof(inet::udp::socket) == 4);
  static_assert(alignof(inet::udp::socket) == 4);
}

TEST(net_inet_udp_socket, nonblocking_send_receive_error) {
  inet::udp::socket socket;
  socket.set_nonblock(true);
  socket.set_reuseaddr(true);
  socket.set_reuseport(true);

  std::vector<std::byte> bytes;
  EXPECT_ANY_THROW(socket.send(bytes));
  EXPECT_EQ(socket.receive(bytes), 0);
}

TEST(net_inet_udp_socket, bind) {
  const inet::sockaddr sockaddr(inet::ip::kLoopback(), inet::port(9999));

  inet::udp::socket socket;
  socket.set_reuseaddr(true);
  socket.set_reuseport(true);
  socket.bind(sockaddr);
  EXPECT_ANY_THROW(socket.bind(sockaddr));
}

TEST(net_inet_udp_socket, connect) {
  const inet::sockaddr sockaddr(inet::ip::kLoopback(), inet::port(9998));

  inet::udp::socket socket;
  socket.set_reuseaddr(true);
  socket.set_reuseport(true);

  EXPECT_EQ(socket.connect(sockaddr), sandbox::net::connect_status::kSuccess);
  EXPECT_EQ(socket.connect(sockaddr), sandbox::net::connect_status::kSuccess);
}

TEST(net_inet_udp_socket, sockaddrs) {
  const inet::sockaddr bind_sockaddr(inet::ip::kLoopback(), inet::port(9997));
  const inet::sockaddr connect_sockaddr(inet::ip::kLoopback(),
                                        inet::port(9996));

  inet::udp::socket socket;
  socket.set_reuseaddr(true);
  socket.set_reuseport(true);

  inet::sockaddr out_sockaddr(inet::ip::kAny(), inet::port(0));
  EXPECT_NO_THROW(socket.get_bind_sockaddr(out_sockaddr));
  EXPECT_NE((socket.get_bind_sockaddr(out_sockaddr), out_sockaddr),
            bind_sockaddr);
  socket.bind(bind_sockaddr);
  EXPECT_EQ((socket.get_bind_sockaddr(out_sockaddr), out_sockaddr),
            bind_sockaddr);
  EXPECT_ANY_THROW(socket.get_connect_sockaddr(out_sockaddr));

  EXPECT_EQ(socket.connect(connect_sockaddr),
            sandbox::net::connect_status::kSuccess);
  EXPECT_EQ((socket.get_connect_sockaddr(out_sockaddr), out_sockaddr),
            connect_sockaddr);
}

TEST(net_inet_udp_socket, blocking_echo_handshake) {
  const std::vector<std::byte> kBuffer{std::byte(1), std::byte(2),
                                       std::byte(3)};

  const inet::sockaddr server_sockaddr(inet::ip::kLoopback(), inet::port(9995));
  const inet::sockaddr client_sockaddr(inet::ip::kLoopback(), inet::port(9996));

  inet::udp::socket server;
  server.set_reuseaddr(true);
  server.set_reuseport(true);
  server.bind(server_sockaddr);

  inet::udp::socket client;
  client.set_reuseaddr(true);
  client.set_reuseport(true);
  client.bind(client_sockaddr);

  const std::jthread server_thread([&server, &client_sockaddr, &kBuffer] {
    inet::sockaddr peer(inet::ip::kAny(), inet::port(0));
    std::vector<std::byte> buffer(kBuffer.size());
    EXPECT_EQ(server.receive_from(buffer, peer), kBuffer.size());
    EXPECT_EQ(buffer, kBuffer);
    EXPECT_EQ(peer, client_sockaddr);
    EXPECT_EQ(server.send_to(kBuffer, peer), kBuffer.size());

    EXPECT_EQ(server.receive(buffer), kBuffer.size());
    EXPECT_EQ(server.connect(client_sockaddr),
              sandbox::net::connect_status::kSuccess);
    EXPECT_EQ(server.send(kBuffer), kBuffer.size());
  });

  inet::sockaddr peer(inet::ip::kAny(), inet::port(0));
  std::vector<std::byte> buffer(kBuffer.size());
  EXPECT_EQ(client.send_to(kBuffer, server_sockaddr), kBuffer.size());
  EXPECT_EQ(client.receive_from(buffer, peer), kBuffer.size());
  EXPECT_EQ(buffer, kBuffer);
  EXPECT_EQ(peer, server_sockaddr);
  EXPECT_EQ(client.connect(server_sockaddr),
            sandbox::net::connect_status::kSuccess);
  EXPECT_EQ(client.send(kBuffer), kBuffer.size());
  EXPECT_EQ(client.receive(buffer), kBuffer.size());
}

TEST(net_inet_udp_socket, nonblocking_echo_handshake) {
  const std::vector<std::byte> kBuffer{std::byte(1), std::byte(2),
                                       std::byte(3)};

  const inet::sockaddr server_sockaddr(inet::ip::kLoopback(), inet::port(9995));
  const inet::sockaddr client_sockaddr(inet::ip::kLoopback(), inet::port(9996));

  inet::udp::socket server;
  server.set_nonblock(true);
  server.set_reuseaddr(true);
  server.set_reuseport(true);
  server.bind(server_sockaddr);

  inet::udp::socket client;
  client.set_nonblock(true);
  client.set_reuseaddr(true);
  client.set_reuseport(true);
  client.bind(client_sockaddr);

  std::vector<std::byte> buffer(kBuffer.size());
  inet::sockaddr peer(inet::ip::kNonRoutable(), inet::port(0));
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

}  // namespace tests::net::inet::udp
