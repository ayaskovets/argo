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

TEST(net_inet_udp_socket, unconnected_send_receive) {
  const std::vector<std::byte> data{std::byte(1), std::byte(2), std::byte(3)};
  const inet::sockaddr sockaddr(inet::ip::kLoopback(), inet::port(9999));

  inet::udp::socket socket;
  socket.set_nonblock(true);
  socket.set_reuseaddr(true);
  socket.set_reuseport(true);

  EXPECT_EQ(socket.send_to(data, sockaddr), data.size());
  std::vector<std::byte> buffer(data.size());
  EXPECT_EQ(socket.receive(buffer), 0);
}

TEST(net_inet_udp_socket, bind) {
  const inet::sockaddr sockaddr(inet::ip::kLoopback(), inet::port(9999));

  inet::udp::socket socket;
  socket.set_reuseaddr(true);
  socket.set_reuseport(true);
  EXPECT_EQ(socket.bind(sockaddr), sandbox::net::bind_status::kSuccess);
  EXPECT_ANY_THROW(socket.bind(sockaddr));

  inet::udp::socket other;
  EXPECT_EQ(other.bind(sockaddr), sandbox::net::bind_status::kInUse);
  other.set_reuseaddr(true);
  other.set_reuseport(true);
  EXPECT_EQ(other.bind(sockaddr), sandbox::net::bind_status::kSuccess);
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

  EXPECT_NE(out_sockaddr, connect_sockaddr);
  EXPECT_EQ(socket.connect(connect_sockaddr),
            sandbox::net::connect_status::kSuccess);
  EXPECT_EQ((socket.get_connect_sockaddr(out_sockaddr), out_sockaddr),
            connect_sockaddr);
}

TEST(net_inet_udp_socket, unconnected_accept) { GTEST_SKIP(); }

TEST(net_inet_udp_socket, blocking_handshake) {
  const std::vector<std::byte> data{std::byte(1), std::byte(2), std::byte(3)};
  const inet::sockaddr server_sockaddr(inet::ip::kLoopback(), inet::port(9995));
  const inet::sockaddr client_sockaddr(inet::ip::kLoopback(), inet::port(9996));

  inet::udp::socket server;
  server.set_reuseaddr(true);
  server.set_reuseport(true);
  server.bind(server_sockaddr);

  const std::jthread server_thread([&server, &client_sockaddr, &data] {
    inet::sockaddr peer(inet::ip::kAny(), inet::port(0));
    std::vector<std::byte> buffer(data.size());
    EXPECT_EQ(server.receive_from(buffer, peer), data.size());
    EXPECT_EQ(buffer, data);
    EXPECT_EQ(peer, client_sockaddr);
    EXPECT_EQ(server.send_to(data, peer), data.size());

    EXPECT_EQ(server.receive(buffer), data.size());
    EXPECT_EQ(server.connect(client_sockaddr),
              sandbox::net::connect_status::kSuccess);
    EXPECT_EQ(server.send(data), data.size());
  });

  inet::sockaddr peer(inet::ip::kAny(), inet::port(0));
  std::vector<std::byte> buffer(data.size());

  inet::udp::socket client;
  client.set_reuseaddr(true);
  client.set_reuseport(true);
  client.bind(client_sockaddr);

  EXPECT_EQ(client.send_to(data, server_sockaddr), data.size());
  EXPECT_EQ(client.receive_from(buffer, peer), data.size());
  EXPECT_EQ(buffer, data);
  EXPECT_EQ(peer, server_sockaddr);

  EXPECT_EQ(client.connect(server_sockaddr),
            sandbox::net::connect_status::kSuccess);
  EXPECT_EQ(client.send(data), data.size());
  EXPECT_EQ(client.receive(buffer), data.size());
  EXPECT_EQ(buffer, data);
}

TEST(net_inet_udp_socket, nonblocking_handshake) {
  const std::vector<std::byte> data{std::byte(1), std::byte(2), std::byte(3)};
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

  inet::sockaddr peer(inet::ip::kNonRoutable(), inet::port(0));
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

}  // namespace tests::net::inet::udp
