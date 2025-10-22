#include <sandbox/net/unix/dgram/socket.hpp>

#include <thread>

#include <gtest/gtest.h>

namespace tests::net::unix::dgram {

TEST(net_unix_dgram_socket, size) {
  static_assert(sizeof(sandbox::net::unix::dgram::socket) == 4);
  static_assert(alignof(sandbox::net::unix::dgram::socket) == 4);
}

TEST(net_unix_dgram_socket, unlink_bind) {
  const sandbox::net::unix::sockaddr sockaddr(
      sandbox::net::unix::sockaddr("net_unix_dgram_socket_unlink_bind"));

  sandbox::net::unix::dgram::socket socket;
  EXPECT_EQ(socket.unlink_bind(sockaddr), sandbox::net::bind_status::kSuccess);

  sandbox::net::unix::dgram::socket other;
  EXPECT_EQ(other.unlink_bind(sockaddr), sandbox::net::bind_status::kSuccess);
}

TEST(net_unix_dgram_socket, unlink_and_close) {
  const sandbox::net::unix::sockaddr sockaddr(
      sandbox::net::unix::sockaddr("net_unix_dgram_socket_close_unlink"));

  sandbox::net::unix::dgram::socket socket;
  EXPECT_EQ(socket.unlink_bind(sockaddr), sandbox::net::bind_status::kSuccess);

  sandbox::net::unix::dgram::socket other;
  EXPECT_EQ(other.bind(sockaddr), sandbox::net::bind_status::kInUse);

  socket.unlink_and_close();
  EXPECT_EQ(other.bind(sockaddr), sandbox::net::bind_status::kSuccess);
}

TEST(net_unix_dgram_socket, not_connected_send_receive) {
  sandbox::net::unix::dgram::socket socket;
  socket.set_nonblock(true);

  std::vector<std::byte> bytes{std::byte(1), std::byte(2), std::byte(3)};
  EXPECT_ANY_THROW(socket.send(bytes));
  EXPECT_EQ(socket.receive(bytes), 0);
}

TEST(net_unix_dgram_socket, blocking_echo_handshake) {
  const std::vector<std::byte> kBuffer{std::byte(1), std::byte(2),
                                       std::byte(3)};

  const sandbox::net::unix::sockaddr server_sockaddr(
      sandbox::net::unix::sockaddr(
          "net_unix_dgram_socket_blocking_echo_handshake_server"));
  const sandbox::net::unix::sockaddr client_sockaddr(
      sandbox::net::unix::sockaddr(
          "net_unix_dgram_socket_blocking_echo_handshake_client"));

  sandbox::net::unix::dgram::socket server;
  EXPECT_EQ(server.unlink_bind(server_sockaddr),
            sandbox::net::bind_status::kSuccess);

  sandbox::net::unix::dgram::socket client;
  EXPECT_EQ(client.unlink_bind(client_sockaddr),
            sandbox::net::bind_status::kSuccess);

  std::jthread server_thread([&server, &client_sockaddr, &kBuffer] {
    sandbox::net::unix::sockaddr peer(sandbox::net::unix::sockaddr::kEmpty());
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

  sandbox::net::unix::sockaddr peer(sandbox::net::unix::sockaddr::kEmpty());
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

TEST(net_unix_dgram_socket, nonblocking_echo_handshake) {
  const std::vector<std::byte> kBuffer{std::byte(1), std::byte(2),
                                       std::byte(3)};

  const sandbox::net::unix::sockaddr server_sockaddr(
      sandbox::net::unix::sockaddr(
          "net_unix_dgram_socket_nonblocking_echo_handshake_server"));
  const sandbox::net::unix::sockaddr client_sockaddr(
      sandbox::net::unix::sockaddr(
          "net_unix_dgram_socket_nonblocking_echo_handshake_client"));

  sandbox::net::unix::dgram::socket server;
  server.set_nonblock(true);
  EXPECT_EQ(server.unlink_bind(server_sockaddr),
            sandbox::net::bind_status::kSuccess);

  sandbox::net::unix::dgram::socket client;
  client.set_nonblock(true);
  EXPECT_EQ(client.unlink_bind(client_sockaddr),
            sandbox::net::bind_status::kSuccess);

  std::vector<std::byte> buffer(kBuffer.size());
  sandbox::net::unix::sockaddr peer(sandbox::net::unix::sockaddr::kEmpty());
  EXPECT_EQ(server.send_to(kBuffer, client_sockaddr), kBuffer.size());
  while (client.receive_from(buffer, peer) != kBuffer.size());
  EXPECT_EQ(buffer, kBuffer);
  EXPECT_EQ(peer, server_sockaddr);

  EXPECT_EQ(server.connect(client_sockaddr),
            sandbox::net::connect_status::kSuccess);
  EXPECT_EQ(client.connect(server_sockaddr),
            sandbox::net::connect_status::kSuccess);

  EXPECT_EQ(client.send(kBuffer), kBuffer.size());
  while (server.receive(buffer) != kBuffer.size());
  EXPECT_EQ(buffer, kBuffer);
}

TEST(net_unix_dgram_socket, get_sockaddrs) {
  sandbox::net::unix::sockaddr out_sockaddr(
      sandbox::net::unix::sockaddr::kEmpty());
  const sandbox::net::unix::sockaddr sockaddr(
      sandbox::net::unix::sockaddr("net_unix_dgram_socket_get_sockaddrs"));

  sandbox::net::unix::dgram::socket server;
  server.unlink_bind(sockaddr);

  sandbox::net::unix::dgram::socket client;
  client.connect(sockaddr);

  EXPECT_EQ((server.get_bind_sockaddr(out_sockaddr), out_sockaddr), sockaddr);
  EXPECT_EQ((client.get_connect_sockaddr(out_sockaddr), out_sockaddr),
            sockaddr);
}

TEST(net_unix_stream_socket, get_unknown_bind_address) {
  sandbox::net::unix::dgram::socket server;

  sandbox::net::unix::sockaddr out_sockaddr(
      sandbox::net::unix::sockaddr::kEmpty());
  EXPECT_NO_THROW(server.get_bind_sockaddr(out_sockaddr));
}

}  // namespace tests::net::unix::dgram
