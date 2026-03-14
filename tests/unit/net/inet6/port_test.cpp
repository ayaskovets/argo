#include <wip/net/inet6/port.hpp>

#include <gtest/gtest.h>

namespace tests::net::inet6 {

namespace inet6 = wip::net::inet6;
namespace net = wip::net;

TEST(net_inet6_port, size) {
  static_assert(sizeof(inet6::port) == 2);
  static_assert(alignof(inet6::port) == 2);
}

TEST(net_inet6_port, construction) {
  EXPECT_NE(inet6::port(1234), inet6::port(1234, net::network_byte_order_t{}));
  EXPECT_EQ(inet6::port(65535),
            inet6::port(65535, net::network_byte_order_t{}));
  EXPECT_EQ(inet6::port(0), inet6::port(0, net::network_byte_order_t{}));
}

}  // namespace tests::net::inet6
