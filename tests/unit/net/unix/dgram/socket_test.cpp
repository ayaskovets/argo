#include <sandbox/net/unix/dgram/socket.hpp>

#include <thread>

#include <gtest/gtest.h>

namespace tests::net::unix::dgram {

namespace unix = sandbox::net::unix;

TEST(net_unix_dgram_socket, size) {
  static_assert(sizeof(unix::dgram::socket) == 4);
  static_assert(alignof(unix::dgram::socket) == 4);
}

TEST(net_unix_dgram_socket, unconnected_send_receive) {
  const std::vector<std::byte> data{std::byte(1), std::byte(2), std::byte(3)};

  unix::dgram::socket socket;
  socket.set_nonblock(true);

  EXPECT_ANY_THROW(socket.send(data));
  std::vector<std::byte> buffer(data.size());
  EXPECT_EQ(socket.receive(buffer), 0);
}

TEST(net_unix_dgram_socket, bind) {
  const unix::sockaddr sockaddr("net_unix_dgram_socket_bind");

  unix::dgram::socket socket;
  EXPECT_EQ(socket.unlink_bind(sockaddr), sandbox::net::bind_status::kSuccess);
  EXPECT_ANY_THROW(socket.unlink_bind(sockaddr));

  unix::dgram::socket other;
  EXPECT_EQ(other.unlink_bind(sockaddr), sandbox::net::bind_status::kSuccess);
  EXPECT_ANY_THROW(other.unlink_bind(sockaddr));
}

TEST(net_unix_dgram_socket, connect) {
  const unix::sockaddr sockaddr("net_unix_dgram_socket_connect");

  unix::dgram::socket socket;

  EXPECT_EQ(socket.connect(sockaddr), sandbox::net::connect_status::kRefused);
  EXPECT_EQ(socket.connect(sockaddr), sandbox::net::connect_status::kRefused);
}

TEST(net_unix_dgram_socket, sockaddrs) {
  unix::sockaddr out_sockaddr(unix::sockaddr::kEmpty());
  const unix::sockaddr sockaddr("net_unix_dgram_socket_sockaddrs");

  unix::dgram::socket server;
  server.unlink_bind(sockaddr);

  unix::dgram::socket client;
  client.connect(sockaddr);

  EXPECT_EQ((server.get_bind_sockaddr(out_sockaddr), out_sockaddr), sockaddr);
  EXPECT_EQ((client.get_connect_sockaddr(out_sockaddr), out_sockaddr),
            sockaddr);
}

TEST(net_unix_dgram_socket, unconnected_accept) { GTEST_SKIP(); }

TEST(net_unix_dgram_socket, blocking_handshake) {
  const std::vector<std::byte> data{std::byte(1), std::byte(2), std::byte(3)};
  const unix::sockaddr server_sockaddr(
      unix::sockaddr("net_unix_dgram_socket_blocking_handshake_server"));
  const unix::sockaddr client_sockaddr(
      unix::sockaddr("net_unix_dgram_socket_blocking_handshake_client"));

  unix::dgram::socket server;
  EXPECT_EQ(server.unlink_bind(server_sockaddr),
            sandbox::net::bind_status::kSuccess);

  const std::jthread server_thread([&server, &client_sockaddr, &data] {
    unix::sockaddr peer(unix::sockaddr::kEmpty());
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

  unix::sockaddr peer(unix::sockaddr::kEmpty());
  std::vector<std::byte> buffer(data.size());

  unix::dgram::socket client;
  EXPECT_EQ(client.unlink_bind(client_sockaddr),
            sandbox::net::bind_status::kSuccess);

  EXPECT_EQ(client.send_to(data, server_sockaddr), data.size());
  EXPECT_EQ(client.receive_from(buffer, peer), data.size());
  EXPECT_EQ(buffer, data);
  EXPECT_EQ(peer, server_sockaddr);

  EXPECT_EQ(client.connect(server_sockaddr),
            sandbox::net::connect_status::kSuccess);
  EXPECT_EQ(client.send(data), data.size());
  EXPECT_EQ(client.receive(buffer), data.size());
}

TEST(net_unix_dgram_socket, nonblocking_handshake) {
  const std::vector<std::byte> data{std::byte(1), std::byte(2), std::byte(3)};

  const unix::sockaddr server_sockaddr(
      unix::sockaddr("net_unix_dgram_socket_nonblocking_handshake_server"));
  const unix::sockaddr client_sockaddr(
      unix::sockaddr("net_unix_dgram_socket_nonblocking_handshake_client"));

  unix::dgram::socket server;
  server.set_nonblock(true);
  EXPECT_EQ(server.unlink_bind(server_sockaddr),
            sandbox::net::bind_status::kSuccess);

  unix::dgram::socket client;
  client.set_nonblock(true);
  EXPECT_EQ(client.unlink_bind(client_sockaddr),
            sandbox::net::bind_status::kSuccess);

  std::vector<std::byte> buffer(data.size());
  unix::sockaddr peer(unix::sockaddr::kEmpty());
  EXPECT_EQ(server.send_to(data, client_sockaddr), data.size());
  while (client.receive_from(buffer, peer) != data.size());
  EXPECT_EQ(buffer, data);
  EXPECT_EQ(peer, server_sockaddr);

  EXPECT_EQ(server.connect(client_sockaddr),
            sandbox::net::connect_status::kSuccess);
  EXPECT_EQ(client.connect(server_sockaddr),
            sandbox::net::connect_status::kSuccess);

  EXPECT_EQ(client.send(data), data.size());
  while (server.receive(buffer) != data.size());
  EXPECT_EQ(buffer, data);
}

TEST(net_unix_stream_socket, get_unknown_bind_address) {
  unix::dgram::socket server;

  unix::sockaddr out_sockaddr(unix::sockaddr::kEmpty());
  EXPECT_NO_THROW(server.get_bind_sockaddr(out_sockaddr));
}

TEST(net_unix_dgram_socket, unlink_close) {
  const unix::sockaddr sockaddr(
      unix::sockaddr("net_unix_dgram_socket_unlink_close"));

  unix::dgram::socket socket;
  EXPECT_EQ(socket.unlink_bind(sockaddr), sandbox::net::bind_status::kSuccess);

  unix::dgram::socket other;
  EXPECT_EQ(other.bind(sockaddr), sandbox::net::bind_status::kInUse);

  socket.unlink_close();
  EXPECT_EQ(other.bind(sockaddr), sandbox::net::bind_status::kSuccess);
}

}  // namespace tests::net::unix::dgram
