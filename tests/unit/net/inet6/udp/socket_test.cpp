#include <sandbox/net/inet6/sockaddr.hpp>
#include <sandbox/net/inet6/udp/socket.hpp>

#include <gtest/gtest.h>

#include "templates/net.hpp"
#include "templates/templates.hpp"

namespace tests::net::inet6::udp {

namespace inet6 = sandbox::net::inet6;
namespace net = sandbox::net;

TEST(net_inet6_udp_socket, size) {
  static_assert(sizeof(inet6::udp::socket) == 4);
  static_assert(alignof(inet6::udp::socket) == 4);
}

TEST_T(net_inet6_udp_socket, dgram_unconnected_send_receive, inet6::udp::socket,
       inet6::sockaddr(inet6::ip::kLoopback(), inet6::port(9999)));

TEST_T(net_inet6_udp_socket, bind, inet6::udp::socket,
       inet6::sockaddr(inet6::ip::kLoopback(), inet6::port(9999)));

TEST_T(net_inet6_udp_socket, connect, inet6::udp::socket,
       inet6::sockaddr(inet6::ip::kLoopback(), inet6::port(9998)),
       net::connect_status::kSuccess);

TEST_T(net_inet6_udp_socket, sockaddrs, inet6::udp::socket,
       inet6::sockaddr(inet6::ip::kLoopback(), inet6::port(9997)),
       inet6::sockaddr(inet6::ip::kLoopback(), inet6::port(9996)),
       net::connect_status::kSuccess);

TEST_T(net_inet6_udp_socket, dgram_blocking_handshake, inet6::udp::socket,
       inet6::sockaddr(inet6::ip::kLoopback(), inet6::port(9995)),
       inet6::sockaddr(inet6::ip::kLoopback(), inet6::port(9996)));

TEST_T(net_inet6_udp_socket, dgram_nonblocking_handshake, inet6::udp::socket,
       inet6::sockaddr(inet6::ip::kLoopback(), inet6::port(9995)),
       inet6::sockaddr(inet6::ip::kLoopback(), inet6::port(9996)));

}  // namespace tests::net::inet6::udp
