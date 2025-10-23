#include <sandbox/net/unix/stream/socket.hpp>

#include <thread>

#include <gtest/gtest.h>

namespace tests::net::unix::stream {

namespace unix = sandbox::net::unix;

TEST(net_unix_stream_socket, size) {
  static_assert(sizeof(unix::stream::socket) == 4);
  static_assert(alignof(unix::stream::socket) == 4);
}

TEST(net_unix_stream_socket, unlink_bind) {
  const unix::sockaddr sockaddr(
      unix::sockaddr("net_unix_stream_socket_unlink_bind"));

  unix::stream::socket socket;
  EXPECT_EQ(socket.unlink_bind(sockaddr), sandbox::net::bind_status::kSuccess);

  unix::stream::socket other;
  EXPECT_EQ(other.unlink_bind(sockaddr), sandbox::net::bind_status::kSuccess);
}

TEST(net_unix_stream_socket, unlink_and_close) {
  const unix::sockaddr sockaddr(
      unix::sockaddr("net_unix_stream_socket_close_unlink"));

  unix::stream::socket socket;
  EXPECT_EQ(socket.unlink_bind(sockaddr), sandbox::net::bind_status::kSuccess);

  unix::stream::socket other;
  EXPECT_EQ(other.bind(sockaddr), sandbox::net::bind_status::kInUse);

  socket.unlink_and_close();
  EXPECT_EQ(other.bind(sockaddr), sandbox::net::bind_status::kSuccess);
}

TEST(net_unix_stream_socket, not_connected_send_receive) {
  unix::stream::socket socket;
  socket.set_nonblock(true);

  std::vector<std::byte> bytes{std::byte(1), std::byte(2), std::byte(3)};
  EXPECT_ANY_THROW(socket.send(bytes));
  EXPECT_ANY_THROW(socket.receive(bytes));
}

TEST(net_unix_stream_socket, blocking_send_receive_error) {
  unix::stream::socket socket;
  socket.set_nonblock(true);

  std::vector<std::byte> bytes;
  EXPECT_ANY_THROW(socket.send(bytes));
  EXPECT_ANY_THROW(socket.receive(bytes));
}

TEST(net_unix_stream_socket, nonblocking_send_receive_error) {
  unix::stream::socket socket;
  socket.set_nonblock(true);

  std::vector<std::byte> bytes;
  EXPECT_ANY_THROW(socket.send(bytes));
  EXPECT_ANY_THROW(socket.receive(bytes));
}

TEST(net_unix_stream_socket, accept_error) {
  unix::stream::socket peer(sandbox::io::uninitialized_t{});
  unix::stream::socket socket;
  socket.set_nonblock(true);

  EXPECT_ANY_THROW(socket.accept(peer));

  EXPECT_ANY_THROW(socket.listen(1));
  const unix::sockaddr sockaddr("net_unix_stream_socket_accept_error");
  socket.unlink_bind(sockaddr);
  EXPECT_NO_THROW(socket.listen(1));

  EXPECT_EQ(socket.accept(peer), sandbox::net::accept_status::kEmptyQueue);
}

TEST(net_unix_stream_socket, blocking_echo_handshake) {
  const std::vector<std::byte> kBuffer{std::byte(1), std::byte(2),
                                       std::byte(3)};

  const unix::sockaddr sockaddr(
      "net_unix_stream_socket_blocking_echo_handshake");

  unix::stream::socket server;
  server.unlink_bind(sockaddr);
  server.listen(1);

  unix::stream::socket client;

  const std::jthread server_thread([&server, &kBuffer] {
    unix::stream::socket peer(sandbox::io::uninitialized_t{});
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

TEST(net_unix_stream_socket, nonblocking_echo_handshake) {
  const std::vector<std::byte> kBuffer{std::byte(1), std::byte(2),
                                       std::byte(3)};

  const unix::sockaddr sockaddr(
      "net_unix_stream_socket_nonblocking_echo_handshake");

  unix::stream::socket server;
  server.set_nonblock(true);

  unix::stream::socket client;
  client.set_nonblock(true);

  server.unlink_bind(sockaddr);
  server.listen(1);
  EXPECT_EQ(client.connect(sockaddr), sandbox::net::connect_status::kSuccess);

  unix::stream::socket peer(sandbox::io::uninitialized_t{});
  EXPECT_EQ(server.accept(peer), sandbox::net::accept_status::kSuccess);

  std::vector<std::byte> buffer(kBuffer.size());
  EXPECT_EQ(peer.send(kBuffer), kBuffer.size());
  while (client.receive(buffer) != kBuffer.size());
  EXPECT_EQ(buffer, kBuffer);
}

TEST(net_unix_stream_socket, get_sockaddrs) {
  const unix::sockaddr sockaddr(
      unix::sockaddr("net_unix_stream_socket_get_sockaddrs"));

  unix::stream::socket server;
  server.set_nonblock(true);
  server.unlink_bind(sockaddr);
  server.listen(1);

  unix::stream::socket client;
  client.set_nonblock(true);
  EXPECT_EQ(client.connect(sockaddr), sandbox::net::connect_status::kSuccess);

  unix::stream::socket peer(sandbox::io::uninitialized_t{});
  EXPECT_EQ(server.accept(peer), sandbox::net::accept_status::kSuccess);

  unix::sockaddr out_sockaddr(unix::sockaddr::kEmpty());
  EXPECT_EQ((server.get_bind_sockaddr(out_sockaddr), out_sockaddr), sockaddr);
  EXPECT_EQ((client.get_connect_sockaddr(out_sockaddr), out_sockaddr),
            sockaddr);
  EXPECT_NO_THROW(client.get_bind_sockaddr(out_sockaddr));
}

}  // namespace tests::net::unix::stream
