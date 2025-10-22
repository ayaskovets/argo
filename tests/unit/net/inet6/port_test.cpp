#include <sandbox/net/inet6/port.hpp>

#include <gtest/gtest.h>

namespace tests::net::inet6 {

TEST(net_inet6_port, size) {
  static_assert(sizeof(sandbox::net::inet6::port) == 2);
  static_assert(alignof(sandbox::net::inet6::port) == 2);
}

TEST(net_inet6_port, construction) {
  EXPECT_NE(
      sandbox::net::inet6::port(1234),
      sandbox::net::inet6::port(1234, sandbox::net::network_byte_order_t{}));
  EXPECT_EQ(
      sandbox::net::inet6::port(65535),
      sandbox::net::inet6::port(65535, sandbox::net::network_byte_order_t{}));
  EXPECT_EQ(sandbox::net::inet6::port(0),
            sandbox::net::inet6::port(0, sandbox::net::network_byte_order_t{}));
}

}  // namespace tests::net::inet6
