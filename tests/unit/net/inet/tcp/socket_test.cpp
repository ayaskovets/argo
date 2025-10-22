#include <sandbox/net/inet/sockaddr.hpp>
#include <sandbox/net/inet/tcp/socket.hpp>

#include <thread>

#include <gtest/gtest.h>

namespace tests::net::inet::tcp {

TEST(net_inet_tcp_socket, size) {
  static_assert(sizeof(sandbox::net::inet::tcp::socket) == 4);
  static_assert(alignof(sandbox::net::inet::tcp::socket) == 4);
}

TEST(net_inet_tcp_socket, blocking_send_receive_error) {
  sandbox::net::inet::tcp::socket socket;
  socket.set_nonblock(true);
  socket.set_reuseaddr(true);
  socket.set_reuseport(true);

  std::vector<std::byte> bytes;
  EXPECT_ANY_THROW(socket.send(bytes));
  EXPECT_ANY_THROW(socket.receive(bytes));
}

TEST(net_inet_tcp_socket, nonblocking_send_receive_error) {
  sandbox::net::inet::tcp::socket socket;
  socket.set_nonblock(true);
  socket.set_reuseaddr(true);
  socket.set_reuseport(true);

  std::vector<std::byte> bytes;
  EXPECT_ANY_THROW(socket.send(bytes));
  EXPECT_ANY_THROW(socket.receive(bytes));
}

TEST(net_inet_tcp_socket, bind) {
  const sandbox::net::inet::sockaddr sockaddr(
      sandbox::net::inet::ip::kLoopback(), sandbox::net::inet::port(9999));

  sandbox::net::inet::tcp::socket socket;
  socket.set_reuseaddr(true);
  socket.set_reuseport(true);
  socket.bind(sockaddr);
  EXPECT_ANY_THROW(socket.bind(sockaddr));
}

TEST(net_inet_tcp_socket, connect) {
  const sandbox::net::inet::sockaddr sockaddr(
      sandbox::net::inet::ip::kLoopback(), sandbox::net::inet::port(9998));

  sandbox::net::inet::tcp::socket socket;
  socket.set_reuseaddr(true);
  socket.set_reuseport(true);

  EXPECT_EQ(socket.connect(sockaddr), sandbox::net::connect_status::kRefused);
  EXPECT_ANY_THROW(socket.connect(sockaddr));
}

TEST(net_inet_tcp_socket, sockaddrs) {
  const sandbox::net::inet::sockaddr bind_sockaddr(
      sandbox::net::inet::ip::kLoopback(), sandbox::net::inet::port(9997));
  const sandbox::net::inet::sockaddr connect_sockaddr(
      sandbox::net::inet::ip::kLoopback(), sandbox::net::inet::port(9996));

  sandbox::net::inet::tcp::socket socket;
  socket.set_nonblock(true);
  socket.set_reuseaddr(true);
  socket.set_reuseport(true);

  sandbox::net::inet::sockaddr out_sockaddr(sandbox::net::inet::ip::kAny(),
                                            sandbox::net::inet::port(0));
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

TEST(net_inet_tcp_socket, accept_error) {
  sandbox::net::inet::tcp::socket socket;
  socket.set_nonblock(true);
  socket.set_reuseaddr(true);
  socket.set_reuseport(true);

  sandbox::net::inet::tcp::socket peer(sandbox::io::uninitialized_t{});
  EXPECT_ANY_THROW(socket.accept(peer));
  socket.listen(1);
  EXPECT_EQ(socket.accept(peer), sandbox::net::accept_status::kEmptyQueue);
}

TEST(net_inet_tcp_socket, blocking_echo_handshake) {
  const std::vector<std::byte> kBuffer{std::byte(1), std::byte(2),
                                       std::byte(3)};

  const sandbox::net::inet::sockaddr sockaddr(
      sandbox::net::inet::ip::kLoopback(), sandbox::net::inet::port(9995));

  sandbox::net::inet::tcp::socket server;
  server.set_reuseaddr(true);
  server.set_reuseport(true);
  server.bind(sockaddr);
  server.listen(1);

  sandbox::net::inet::tcp::socket client;
  client.set_reuseaddr(true);
  client.set_reuseport(true);

  std::jthread server_thread([&server, &kBuffer] {
    sandbox::net::inet::tcp::socket peer;
    EXPECT_EQ(server.accept(peer), sandbox::net::accept_status::kSuccess);

    std::vector<std::byte> buffer(kBuffer.size());
    EXPECT_EQ(peer.receive(buffer), kBuffer.size());
    EXPECT_EQ(buffer, kBuffer);
    EXPECT_EQ(peer.send(kBuffer), kBuffer.size());
  });

  EXPECT_EQ(client.connect(sockaddr), sandbox::net::connect_status::kSuccess);

  std::vector<std::byte> buffer(kBuffer.size());
  EXPECT_EQ(client.send(kBuffer), kBuffer.size());
  EXPECT_EQ(client.receive(buffer), kBuffer.size());
  EXPECT_EQ(buffer, kBuffer);
}

TEST(net_inet_tcp_socket, nonblocking_echo_handshake) {
  const std::vector<std::byte> kBuffer{std::byte(1), std::byte(2),
                                       std::byte(3)};

  const sandbox::net::inet::sockaddr sockaddr(
      sandbox::net::inet::ip::kLoopback(), sandbox::net::inet::port(9996));

  sandbox::net::inet::tcp::socket server;
  server.set_nonblock(true);
  server.set_reuseaddr(true);
  server.set_reuseport(true);

  sandbox::net::inet::tcp::socket client;
  client.set_nonblock(true);
  client.set_reuseaddr(true);
  client.set_reuseport(true);

  server.bind(sockaddr);
  server.listen(1);
  EXPECT_EQ(client.connect(sockaddr), sandbox::net::connect_status::kPending);

  sandbox::net::inet::tcp::socket peer(sandbox::io::uninitialized_t{});
  while (server.accept(peer) != sandbox::net::accept_status::kSuccess);

  std::vector<std::byte> buffer(kBuffer.size());
  EXPECT_EQ(peer.send(kBuffer), kBuffer.size());
  while (client.receive(buffer) != kBuffer.size());
  EXPECT_EQ(buffer, kBuffer);
}

}  // namespace tests::net::inet::tcp
