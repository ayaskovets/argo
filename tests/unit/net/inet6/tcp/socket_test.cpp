#include <sandbox/net/inet6/sockaddr.hpp>
#include <sandbox/net/inet6/tcp/socket.hpp>

#include <thread>

#include <gtest/gtest.h>

#include "sandbox/net/base_socket.hpp"

namespace tests::net::inet6::tcp {

namespace inet6 = sandbox::net::inet6;

TEST(net_inet6_tcp_socket, size) {
  static_assert(sizeof(inet6::tcp::socket) == 4);
  static_assert(alignof(inet6::tcp::socket) == 4);
}

TEST(net_inet6_tcp_socket, unconnected_send_receive) {
  const std::vector<std::byte> data{std::byte(1), std::byte(2), std::byte(3)};

  inet6::tcp::socket socket;
  socket.set_reuseaddr(true);
  socket.set_reuseport(true);

  EXPECT_ANY_THROW(socket.send(data));
  std::vector<std::byte> buffer(data.size());
  EXPECT_ANY_THROW(socket.receive(buffer));
}

TEST(net_inet6_tcp_socket, bind) {
  const inet6::sockaddr sockaddr(inet6::ip::kLoopback(), inet6::port(9999));

  inet6::tcp::socket socket;
  socket.set_reuseaddr(true);
  socket.set_reuseport(true);
  EXPECT_EQ(socket.bind(sockaddr), sandbox::net::bind_status::kSuccess);
  EXPECT_ANY_THROW(socket.bind(sockaddr));

  inet6::tcp::socket other;
  EXPECT_EQ(other.bind(sockaddr), sandbox::net::bind_status::kInUse);
  other.set_reuseaddr(true);
  other.set_reuseport(true);
  EXPECT_EQ(other.bind(sockaddr), sandbox::net::bind_status::kSuccess);
}

TEST(net_inet6_tcp_socket, connect) {
  const inet6::sockaddr sockaddr(inet6::ip::kLoopback(), inet6::port(9998));

  inet6::tcp::socket socket;
  socket.set_reuseaddr(true);
  socket.set_reuseport(true);

  EXPECT_EQ(socket.connect(sockaddr), sandbox::net::connect_status::kRefused);
  EXPECT_ANY_THROW(socket.connect(sockaddr));
}

TEST(net_inet6_tcp_socket, sockaddrs) {
  const inet6::sockaddr bind_sockaddr(inet6::ip::kLoopback(),
                                      inet6::port(9997));
  const inet6::sockaddr connect_sockaddr(inet6::ip::kLoopback(),
                                         inet6::port(9996));

  inet6::tcp::socket socket;
  socket.set_nonblock(true);
  socket.set_reuseaddr(true);
  socket.set_reuseport(true);

  inet6::sockaddr out_sockaddr(inet6::ip::kAny(), inet6::port(0));
  EXPECT_NO_THROW((socket.get_bind_sockaddr(out_sockaddr)));
  EXPECT_NE((socket.get_bind_sockaddr(out_sockaddr), out_sockaddr),
            bind_sockaddr);
  socket.bind(bind_sockaddr);
  EXPECT_EQ((socket.get_bind_sockaddr(out_sockaddr), out_sockaddr),
            bind_sockaddr);
  EXPECT_ANY_THROW(socket.get_connect_sockaddr(out_sockaddr));

  EXPECT_EQ(socket.connect(connect_sockaddr),
            sandbox::net::connect_status::kPending);
  EXPECT_ANY_THROW(socket.get_connect_sockaddr(out_sockaddr));
}

TEST(net_inet6_tcp_socket, unconnected_accept) {
  inet6::tcp::socket socket;
  socket.set_nonblock(true);
  socket.set_reuseaddr(true);
  socket.set_reuseport(true);

  inet6::tcp::socket peer(sandbox::io::uninitialized_t{});
  EXPECT_ANY_THROW(socket.accept(peer));
  socket.listen(1);
  EXPECT_EQ(socket.accept(peer), sandbox::net::accept_status::kEmptyQueue);
}

TEST(net_inet6_tcp_socket, blocking_handshake) {
  const std::vector<std::byte> data{std::byte(1), std::byte(2), std::byte(3)};
  const inet6::sockaddr sockaddr(inet6::ip::kLoopback(), inet6::port(9995));

  inet6::tcp::socket server;
  server.set_reuseaddr(true);
  server.set_reuseport(true);
  server.bind(sockaddr);
  server.listen(1);

  const std::jthread server_thread([&server, &data] {
    inet6::tcp::socket peer;
    EXPECT_EQ(server.accept(peer), sandbox::net::accept_status::kSuccess);

    std::vector<std::byte> buffer(data.size());
    EXPECT_EQ(peer.receive(buffer), data.size());
    EXPECT_EQ(buffer, data);
    EXPECT_EQ(peer.send(data), data.size());
  });

  inet6::tcp::socket client;
  client.set_reuseaddr(true);
  client.set_reuseport(true);

  EXPECT_EQ(client.connect(sockaddr), sandbox::net::connect_status::kSuccess);
  EXPECT_EQ(client.send(data), data.size());
  std::vector<std::byte> buffer(data.size());
  EXPECT_EQ(client.receive(buffer), data.size());
  EXPECT_EQ(buffer, data);
}

TEST(net_inet6_tcp_socket, nonblocking_handshake) {
  const std::vector<std::byte> data{std::byte(1), std::byte(2), std::byte(3)};
  const inet6::sockaddr sockaddr(inet6::ip::kLoopback(), inet6::port(9996));

  inet6::tcp::socket server;
  server.set_nonblock(true);
  server.set_reuseaddr(true);
  server.set_reuseport(true);
  server.bind(sockaddr);
  server.listen(1);

  inet6::tcp::socket client;
  client.set_nonblock(true);
  client.set_reuseaddr(true);
  client.set_reuseport(true);

  EXPECT_EQ(client.connect(sockaddr), sandbox::net::connect_status::kPending);
  inet6::tcp::socket peer(sandbox::io::uninitialized_t{});
  while (server.accept(peer) != sandbox::net::accept_status::kSuccess);

  EXPECT_EQ(peer.send(data), data.size());
  std::vector<std::byte> buffer(data.size());
  while (client.receive(buffer) != data.size());
  EXPECT_EQ(buffer, data);
}

}  // namespace tests::net::inet6::tcp
