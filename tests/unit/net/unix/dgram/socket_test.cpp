#include <wip/net/unix/dgram/socket.hpp>

#include <gtest/gtest.h>

#include "templates/net.hpp"
#include "templates/templates.hpp"

namespace tests::net::unix::dgram {

namespace io = wip::io;
namespace net = wip::net;
namespace unix = wip::net::unix;

TEST(net_unix_dgram_socket, size) {
  static_assert(sizeof(unix::dgram::socket) == 4);
  static_assert(alignof(unix::dgram::socket) == 4);
}

TEST(net_unix_dgram_socket, dgram_unconnected_send_receive) {
  const std::vector<std::byte> data{std::byte(1), std::byte(2), std::byte(3)};

  unix::dgram::socket socket;
  socket.set_flags(io::fd::flag::kNonblock);

  EXPECT_ANY_THROW(socket.send(data));
  std::vector<std::byte> buffer(data.size());
  EXPECT_EQ(socket.receive(buffer), 0);
}

TEST_T(net_unix_dgram_socket, bind, unix::dgram::socket,
       unix::sockaddr("net_unix_dgram_socket_bind"));

TEST_T(net_unix_dgram_socket, connect, unix::dgram::socket,
       unix::sockaddr("net_unix_dgram_socket_connect"),
       net::connect_status::kRefused);

TEST_T(net_unix_dgram_socket, sockaddrs, unix::dgram::socket,
       unix::sockaddr("net_unix_dgram_socket_bind_sockaddr"),
       unix::sockaddr("net_unix_dgram_socket_connect_sockaddr"),
       net::connect_status::kRefused);

TEST_T(net_unix_dgram_socket, dgram_blocking_handshake, unix::dgram::socket,
       unix::sockaddr("net_unix_dgram_socket_blocking_handshake_server"),
       unix::sockaddr("net_unix_dgram_socket_blocking_handshake_client"));

TEST_T(net_unix_dgram_socket, dgram_nonblocking_handshake, unix::dgram::socket,
       unix::sockaddr("net_unix_dgram_socket_nonblocking_handshake_server"),
       unix::sockaddr("net_unix_dgram_socket_nonblocking_handshake_client"));

TEST(net_unix_dgram_socket, unlink) {
  const unix::sockaddr sockaddr(unix::sockaddr("net_unix_dgram_socket_unlink"));

  unix::dgram::socket socket;
  EXPECT_EQ(socket.unlink_bind(sockaddr), net::bind_status::kSuccess);

  unix::dgram::socket other;
  EXPECT_EQ(other.bind(sockaddr), net::bind_status::kInUse);

  socket.unlink_close();
  EXPECT_EQ(other.bind(sockaddr), net::bind_status::kSuccess);

  unix::dgram::socket reuseaddr_socket;
  EXPECT_EQ(reuseaddr_socket.bind(sockaddr), net::bind_status::kInUse);
  reuseaddr_socket.set_reuseaddr(true);
  EXPECT_EQ(reuseaddr_socket.bind(sockaddr), net::bind_status::kSuccess);
}

}  // namespace tests::net::unix::dgram
