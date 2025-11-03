#include <sandbox/net/inet6/sockaddr.hpp>
#include <sandbox/net/inet6/tcp/socket.hpp>

#include <gtest/gtest.h>

#include "templates/net.hpp"
#include "templates/templates.hpp"

namespace tests::net::inet6::tcp {

namespace inet6 = sandbox::net::inet6;
namespace net = sandbox::net;

TEST(net_inet6_tcp_socket, size) {
  static_assert(sizeof(inet6::tcp::socket) == 4);
  static_assert(alignof(inet6::tcp::socket) == 4);
}

TEST_T(net_inet6_tcp_socket, unconnected_send_receive, inet6::tcp::socket);

TEST_T(net_inet6_tcp_socket, bind, inet6::tcp::socket,
       inet6::sockaddr(inet6::ip::kLoopback(), inet6::port(9999)));

TEST_T(net_inet6_tcp_socket, connect, inet6::tcp::socket,
       inet6::sockaddr(inet6::ip::kLoopback(), inet6::port(9998)));

TEST_T(net_inet6_tcp_socket, sockaddrs, inet6::tcp::socket,
       inet6::sockaddr(inet6::ip::kLoopback(), inet6::port(9997)),
       inet6::sockaddr(inet6::ip::kLoopback(), inet6::port(9996)));

TEST_T(net_inet6_tcp_socket, unconnected_accept, inet6::tcp::socket,
       inet6::sockaddr(inet6::ip::kLoopback(), inet6::port(9999)));

TEST_T(net_inet6_tcp_socket, blocking_handshake, inet6::tcp::socket,
       inet6::sockaddr(inet6::ip::kLoopback(), inet6::port(9995)));

TEST_T(net_inet6_tcp_socket, nonblocking_handshake, inet6::tcp::socket,
       inet6::sockaddr(inet6::ip::kLoopback(), inet6::port(9996)),
       net::connect_status::kPending);

}  // namespace tests::net::inet6::tcp
