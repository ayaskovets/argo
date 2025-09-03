#include "net/unix/stream/socket.hpp"

#include <gtest/gtest.h>

#include <thread>

namespace tests::ipc::unix::stream {

TEST(net_unix_stream_socket, size) {
  static_assert(sizeof(core::net::unix::stream::socket) == 4);
  static_assert(alignof(core::net::unix::stream::socket) == 4);
}

TEST(net_unix_stream_socket, unlink_bind) {
  const core::net::unix::sockaddr sockaddr(
      core::net::unix::sockaddr("net_unix_stream_socket_unlink_bind"));

  core::net::unix::stream::socket socket;
  EXPECT_EQ(socket.unlink_bind(sockaddr),
            core::net::unix::stream::socket::bind_status::kSuccess);

  core::net::unix::stream::socket other;
  EXPECT_EQ(other.unlink_bind(sockaddr),
            core::net::unix::stream::socket::bind_status::kSuccess);
}

TEST(net_unix_stream_socket, unlink_and_close) {
  const core::net::unix::sockaddr sockaddr(
      core::net::unix::sockaddr("net_unix_stream_socket_close_unlink"));

  core::net::unix::stream::socket socket;
  EXPECT_EQ(socket.unlink_bind(sockaddr),
            core::net::unix::stream::socket::bind_status::kSuccess);

  core::net::unix::stream::socket other;
  EXPECT_EQ(other.bind(sockaddr),
            core::net::unix::stream::socket::bind_status::kInUse);

  socket.unlink_and_close();
  EXPECT_EQ(other.bind(sockaddr),
            core::net::unix::stream::socket::bind_status::kSuccess);
}

TEST(net_unix_stream_socket, not_connected_send_receive) {
  core::net::unix::stream::socket socket;
  socket.set_nonblock(true);

  std::vector<std::byte> bytes{std::byte(1), std::byte(2), std::byte(3)};
  EXPECT_ANY_THROW(socket.send(bytes));
  EXPECT_ANY_THROW(socket.receive(bytes));
}

TEST(net_unix_stream_socket, blocking_send_receive_error) {
  core::net::unix::stream::socket socket;
  socket.set_nonblock(true);

  std::vector<std::byte> bytes;
  EXPECT_ANY_THROW(socket.send(bytes));
  EXPECT_ANY_THROW(socket.receive(bytes));
}

TEST(net_unix_stream_socket, nonblocking_send_receive_error) {
  core::net::unix::stream::socket socket;
  socket.set_nonblock(true);

  std::vector<std::byte> bytes;
  EXPECT_ANY_THROW(socket.send(bytes));
  EXPECT_ANY_THROW(socket.receive(bytes));
}

TEST(net_unix_stream_socket, accept_error) {
  core::net::unix::stream::socket peer(core::io::fd::uninitialized_t{});
  core::net::unix::stream::socket socket;
  socket.set_nonblock(true);

  EXPECT_ANY_THROW(socket.accept(peer));

  EXPECT_ANY_THROW(socket.listen(1));
  const core::net::unix::sockaddr sockaddr(
      "net_unix_stream_socket_accept_error");
  socket.unlink_bind(sockaddr);
  EXPECT_NO_THROW(socket.listen(1));

  EXPECT_EQ(socket.accept(peer),
            core::net::unix::stream::socket::accept_status::kEmptyQueue);
}

TEST(net_unix_stream_socket, blocking_echo_handshake) {
  const std::vector<std::byte> kBuffer{std::byte(1), std::byte(2),
                                       std::byte(3)};

  const core::net::unix::sockaddr sockaddr(
      "net_unix_stream_socket_blocking_echo_handshake");

  core::net::unix::stream::socket server;
  server.unlink_bind(sockaddr);
  server.listen(1);

  core::net::unix::stream::socket client;

  std::thread server_thread([&server, &kBuffer] {
    core::net::unix::stream::socket peer(core::io::fd::uninitialized_t{});
    EXPECT_EQ(server.accept(peer),
              core::net::unix::stream::socket::accept_status::kSuccess);

    std::vector<std::byte> buffer(kBuffer.size());
    EXPECT_EQ(peer.receive(buffer), kBuffer.size());
    EXPECT_EQ(buffer, kBuffer);
    EXPECT_EQ(peer.send(kBuffer), kBuffer.size());
  });

  EXPECT_EQ(client.connect(sockaddr),
            core::net::unix::stream::socket::connection_status::kSuccess);

  std::vector<std::byte> buffer(kBuffer.size());
  EXPECT_EQ(client.send(kBuffer), kBuffer.size());
  EXPECT_EQ(client.receive(buffer), kBuffer.size());
  EXPECT_EQ(buffer, kBuffer);
  server_thread.join();
}

TEST(net_unix_stream_socket, nonblocking_echo_handshake) {
  const std::vector<std::byte> kBuffer{std::byte(1), std::byte(2),
                                       std::byte(3)};

  const core::net::unix::sockaddr sockaddr(
      "net_unix_stream_socket_nonblocking_echo_handshake");

  core::net::unix::stream::socket server;
  server.set_nonblock(true);

  core::net::unix::stream::socket client;
  client.set_nonblock(true);

  server.unlink_bind(sockaddr);
  server.listen(1);
  EXPECT_EQ(client.connect(sockaddr),
            core::net::unix::stream::socket::connection_status::kSuccess);

  core::net::unix::stream::socket peer(core::io::fd::uninitialized_t{});
  EXPECT_EQ(server.accept(peer),
            core::net::unix::stream::socket::accept_status::kSuccess);

  std::vector<std::byte> buffer(kBuffer.size());
  EXPECT_EQ(peer.send(kBuffer), kBuffer.size());
  while (client.receive(buffer) != kBuffer.size());
  EXPECT_EQ(buffer, kBuffer);
}

TEST(net_unix_stream_socket, get_sockaddrs) {
  const core::net::unix::sockaddr sockaddr(
      core::net::unix::sockaddr("net_unix_stream_socket_get_sockaddrs"));

  core::net::unix::stream::socket server;
  server.set_nonblock(true);
  server.unlink_bind(sockaddr);
  server.listen(1);

  core::net::unix::stream::socket client;
  client.set_nonblock(true);
  EXPECT_EQ(client.connect(sockaddr),
            core::net::unix::stream::socket::connection_status::kSuccess);

  core::net::unix::stream::socket peer(core::io::fd::uninitialized_t{});
  EXPECT_EQ(server.accept(peer),
            core::net::unix::stream::socket::accept_status::kSuccess);

  core::net::unix::sockaddr out_sockaddr(core::net::unix::sockaddr::kEmpty());
  EXPECT_EQ((server.get_bind_sockaddr(out_sockaddr), out_sockaddr), sockaddr);
  EXPECT_EQ((client.get_connect_sockaddr(out_sockaddr), out_sockaddr),
            sockaddr);
  EXPECT_NO_THROW(client.get_bind_sockaddr(out_sockaddr));
}

}  // namespace tests::ipc::unix::stream
