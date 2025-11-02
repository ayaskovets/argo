#include <sandbox/net/unix/stream/socket.hpp>

#include <thread>

#include <gtest/gtest.h>

namespace tests::net::unix::stream {

namespace io = sandbox::io;
namespace net = sandbox::net;
namespace unix = sandbox::net::unix;

TEST(net_unix_stream_socket, size) {
  static_assert(sizeof(unix::stream::socket) == 4);
  static_assert(alignof(unix::stream::socket) == 4);
}

TEST(net_inet_tcp_socket, unconnected_send_receive) {
  const std::vector<std::byte> data{std::byte(1), std::byte(2), std::byte(3)};

  unix::stream::socket socket;

  EXPECT_ANY_THROW(socket.send(data));
  std::vector<std::byte> buffer(data.size());
  EXPECT_ANY_THROW(socket.receive(buffer));
}

TEST(net_unix_stream_socket, bind) {
  const unix::sockaddr sockaddr("net_unix_stream_socket_bind");

  unix::stream::socket socket;
  EXPECT_EQ(socket.unlink_bind(sockaddr), net::bind_status::kSuccess);
  EXPECT_ANY_THROW(socket.unlink_bind(sockaddr));

  unix::stream::socket other;
  EXPECT_EQ(other.unlink_bind(sockaddr), net::bind_status::kSuccess);
  EXPECT_ANY_THROW(other.unlink_bind(sockaddr));
}

TEST(net_unix_stream_socket, connect) {
  const unix::sockaddr sockaddr(
      unix::sockaddr("net_unix_stream_socket_connect"));

  unix::stream::socket socket;
  socket.set_reuseaddr(true);
  socket.set_reuseport(true);

  EXPECT_EQ(socket.connect(sockaddr), net::connect_status::kRefused);
  EXPECT_EQ(socket.connect(sockaddr), net::connect_status::kRefused);
}

TEST(net_unix_stream_socket, sockaddrs) {
  const unix::sockaddr sockaddr(
      unix::sockaddr("net_unix_stream_socket_sockaddrs"));

  unix::stream::socket server;
  server.unlink_bind(sockaddr);
  server.listen(1);

  unix::stream::socket client;
  EXPECT_EQ(client.connect(sockaddr), net::connect_status::kSuccess);

  unix::stream::socket peer(io::uninitialized_t{});
  EXPECT_EQ(server.accept(peer), net::accept_status::kSuccess);

  unix::sockaddr out_sockaddr(unix::sockaddr::kEmpty());
  EXPECT_NE(out_sockaddr, sockaddr);
  EXPECT_EQ((server.get_bind_sockaddr(out_sockaddr), out_sockaddr), sockaddr);
  EXPECT_EQ((client.get_connect_sockaddr(out_sockaddr), out_sockaddr),
            sockaddr);
  EXPECT_NO_THROW(client.get_bind_sockaddr(out_sockaddr));
}

TEST(net_unix_stream_socket, unconnected_accept) {
  unix::stream::socket peer(io::uninitialized_t{});

  unix::stream::socket socket;
  socket.set_flags(io::fd::flag::kNonblock);

  EXPECT_ANY_THROW(socket.listen(1));
  EXPECT_ANY_THROW(socket.accept(peer));

  const unix::sockaddr sockaddr("net_unix_stream_socket_unconnected_accept");
  socket.unlink_bind(sockaddr);

  EXPECT_NO_THROW(socket.listen(1));
  EXPECT_EQ(socket.accept(peer), net::accept_status::kEmptyQueue);
}

TEST(net_unix_stream_socket, blocking_handshake) {
  const std::vector<std::byte> data{std::byte(1), std::byte(2), std::byte(3)};
  const unix::sockaddr sockaddr("net_unix_stream_socket_blocking_handshake");

  unix::stream::socket server;
  server.unlink_bind(sockaddr);
  server.listen(1);

  const std::jthread server_thread([&server, &data] {
    unix::stream::socket peer(io::uninitialized_t{});
    EXPECT_EQ(server.accept(peer), net::accept_status::kSuccess);

    std::vector<std::byte> buffer(data.size());
    EXPECT_EQ(peer.receive(buffer), data.size());
    EXPECT_EQ(buffer, data);
    EXPECT_EQ(peer.send(data), data.size());
  });

  unix::stream::socket client;

  EXPECT_EQ(client.connect(sockaddr), net::connect_status::kSuccess);
  EXPECT_EQ(client.send(data), data.size());
  std::vector<std::byte> buffer(data.size());
  EXPECT_EQ(client.receive(buffer), data.size());
  EXPECT_EQ(buffer, data);
}

TEST(net_unix_stream_socket, nonblocking_handshake) {
  const std::vector<std::byte> data{std::byte(1), std::byte(2), std::byte(3)};
  const unix::sockaddr sockaddr("net_unix_stream_socket_nonblocking_handshake");

  unix::stream::socket server;
  server.set_flags(io::fd::flag::kNonblock);
  server.unlink_bind(sockaddr);
  server.listen(1);

  unix::stream::socket client;
  client.set_flags(io::fd::flag::kNonblock);

  EXPECT_EQ(client.connect(sockaddr), net::connect_status::kSuccess);
  unix::stream::socket peer(io::uninitialized_t{});
  EXPECT_EQ(server.accept(peer), net::accept_status::kSuccess);

  EXPECT_EQ(peer.send(data), data.size());
  std::vector<std::byte> buffer(data.size());
  while (client.receive(buffer) != data.size());
  EXPECT_EQ(buffer, data);
}

TEST(net_unix_stream_socket, unlink_close) {
  const unix::sockaddr sockaddr(
      unix::sockaddr("net_unix_stream_socket_unlink_close"));

  unix::stream::socket socket;
  EXPECT_EQ(socket.unlink_bind(sockaddr), net::bind_status::kSuccess);

  unix::stream::socket other;
  EXPECT_EQ(other.bind(sockaddr), net::bind_status::kInUse);

  socket.unlink_close();
  EXPECT_EQ(other.bind(sockaddr), net::bind_status::kSuccess);

  unix::stream::socket reuseaddr_socket;
  EXPECT_EQ(reuseaddr_socket.bind(sockaddr), net::bind_status::kInUse);
  reuseaddr_socket.set_reuseaddr(true);
  EXPECT_EQ(reuseaddr_socket.bind(sockaddr), net::bind_status::kSuccess);
}

}  // namespace tests::net::unix::stream
