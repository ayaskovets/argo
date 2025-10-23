#include <sandbox/net/inet/port.hpp>

#include <gtest/gtest.h>

namespace tests::net::inet {

namespace inet = sandbox::net::inet;

TEST(net_inet_port, size) {
  static_assert(sizeof(inet::port) == 2);
  static_assert(alignof(inet::port) == 2);
}

TEST(net_inet_port, construction) {
  EXPECT_NE(inet::port(1234),
            inet::port(1234, sandbox::net::network_byte_order_t{}));
  EXPECT_EQ(inet::port(65535),
            inet::port(65535, sandbox::net::network_byte_order_t{}));
  EXPECT_EQ(inet::port(0), inet::port(0, sandbox::net::network_byte_order_t{}));
}

}  // namespace tests::net::inet
