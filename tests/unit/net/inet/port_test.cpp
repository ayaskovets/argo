#include <sandbox/net/inet/port.hpp>

#include <gtest/gtest.h>

namespace tests::net::inet {

TEST(net_inet_port, size) {
  static_assert(sizeof(sandbox::net::inet::port) == 2);
  static_assert(alignof(sandbox::net::inet::port) == 2);
}

TEST(net_inet_port, construction) {
  EXPECT_NE(
      sandbox::net::inet::port(1234),
      sandbox::net::inet::port(1234, sandbox::net::network_byte_order_t{}));
  EXPECT_EQ(
      sandbox::net::inet::port(65535),
      sandbox::net::inet::port(65535, sandbox::net::network_byte_order_t{}));
  EXPECT_EQ(sandbox::net::inet::port(0),
            sandbox::net::inet::port(0, sandbox::net::network_byte_order_t{}));
}

}  // namespace tests::net::inet
