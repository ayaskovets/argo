#include <sandbox/net/unix/stream/socket.hpp>

#include <gtest/gtest.h>

#include "templates/net.hpp"
#include "templates/templates.hpp"

namespace tests::net::unix::stream {

namespace net = sandbox::net;
namespace unix = sandbox::net::unix;

TEST(net_unix_stream_socket, size) {
  static_assert(sizeof(unix::stream::socket) == 4);
  static_assert(alignof(unix::stream::socket) == 4);
}

TEST_T(net_unix_stream_socket, stream_unconnected_send_receive,
       unix::stream::socket);

TEST_T(net_unix_stream_socket, bind, unix::stream::socket,
       unix::sockaddr("net_unix_stream_socket_bind"));

TEST_T(net_unix_stream_socket, connect, unix::stream::socket,
       unix::sockaddr("net_unix_stream_socket_connect"),
       net::connect_status::kRefused);

TEST_T(net_unix_stream_socket, sockaddrs, unix::stream::socket,
       unix::sockaddr("net_unix_stream_socket_bind_sockaddr"),
       unix::sockaddr("net_unix_stream_socket_connect_sockaddr"),
       net::connect_status::kRefused);

TEST_T(net_unix_stream_socket, unconnected_accept, unix::stream::socket,
       unix::sockaddr("net_unix_stream_socket_unconnected_accept"));

TEST_T(net_unix_stream_socket, stream_blocking_handshake, unix::stream::socket,
       unix::sockaddr("net_unix_stream_socket_blocking_handshake"));

TEST_T(net_unix_stream_socket, stream_nonblocking_handshake,
       unix::stream::socket,
       unix::sockaddr("net_unix_stream_socket_nonblocking_handshake"),
       net::connect_status::kSuccess);

TEST(net_unix_stream_socket, unlink) {
  const unix::sockaddr sockaddr(
      unix::sockaddr("net_unix_stream_socket_unlink"));

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
