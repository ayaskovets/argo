#include <wip/net/inet/sockaddr.hpp>
#include <wip/net/inet/tcp/socket.hpp>

#include <gtest/gtest.h>

#include "templates/net.hpp"
#include "templates/templates.hpp"

namespace tests::net::inet::tcp {

namespace inet = wip::net::inet;
namespace net = wip::net;

TEST(net_inet_tcp_socket, size) {
  static_assert(sizeof(inet::tcp::socket) == 4);
  static_assert(alignof(inet::tcp::socket) == 4);
}

TEST_T(net_inet_tcp_socket, stream_unconnected_send_receive, inet::tcp::socket);

TEST_T(net_inet_tcp_socket, bind, inet::tcp::socket,
       inet::sockaddr(inet::ip::kLoopback(), inet::port(9999)));

TEST_T(net_inet_tcp_socket, connect, inet::tcp::socket,
       inet::sockaddr(inet::ip::kLoopback(), inet::port(9998)),
       net::connect_status::kRefused);

TEST_T(net_inet_tcp_socket, sockaddrs, inet::tcp::socket,
       inet::sockaddr(inet::ip::kLoopback(), inet::port(9997)),
       inet::sockaddr(inet::ip::kLoopback(), inet::port(9996)),
       net::connect_status::kPending);

TEST_T(net_inet_tcp_socket, unconnected_accept, inet::tcp::socket,
       inet::sockaddr(inet::ip::kLoopback(), inet::port(9999)));

TEST_T(net_inet_tcp_socket, stream_blocking_handshake, inet::tcp::socket,
       inet::sockaddr(inet::ip::kLoopback(), inet::port(9995)));

TEST_T(net_inet_tcp_socket, stream_nonblocking_handshake, inet::tcp::socket,
       inet::sockaddr(inet::ip::kLoopback(), inet::port(9996)),
       net::connect_status::kPending);

}  // namespace tests::net::inet::tcp
