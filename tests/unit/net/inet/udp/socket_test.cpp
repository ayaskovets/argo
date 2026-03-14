#include <wip/net/inet/sockaddr.hpp>
#include <wip/net/inet/udp/socket.hpp>

#include <gtest/gtest.h>

#include "templates/net.hpp"
#include "templates/templates.hpp"

namespace tests::net::inet::udp {

namespace inet = wip::net::inet;
namespace net = wip::net;

TEST(net_inet_udp_socket, size) {
  static_assert(sizeof(inet::udp::socket) == 4);
  static_assert(alignof(inet::udp::socket) == 4);
}

TEST_T(net_inet_udp_socket, dgram_unconnected_send_receive, inet::udp::socket,
       inet::sockaddr(inet::ip::kLoopback(), inet::port(9999)));

TEST_T(net_inet_udp_socket, bind, inet::udp::socket,
       inet::sockaddr(inet::ip::kLoopback(), inet::port(9999)));

TEST_T(net_inet_udp_socket, connect, inet::udp::socket,
       inet::sockaddr(inet::ip::kLoopback(), inet::port(9998)),
       net::connect_status::kSuccess);

TEST_T(net_inet_udp_socket, sockaddrs, inet::udp::socket,
       inet::sockaddr(inet::ip::kLoopback(), inet::port(9997)),
       inet::sockaddr(inet::ip::kLoopback(), inet::port(9996)),
       net::connect_status::kSuccess);

TEST_T(net_inet_udp_socket, dgram_blocking_handshake, inet::udp::socket,
       inet::sockaddr(inet::ip::kLoopback(), inet::port(9995)),
       inet::sockaddr(inet::ip::kLoopback(), inet::port(9996)));

TEST_T(net_inet_udp_socket, dgram_nonblocking_handshake, inet::udp::socket,
       inet::sockaddr(inet::ip::kLoopback(), inet::port(9995)),
       inet::sockaddr(inet::ip::kLoopback(), inet::port(9996)));

}  // namespace tests::net::inet::udp
