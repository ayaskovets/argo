#include "net/inet6/port.hpp"

#include <gtest/gtest.h>

namespace tests::net::inet6 {

TEST(net_inet6_port, size) {
  static_assert(sizeof(core::net::inet6::port) == 2);
  static_assert(alignof(core::net::inet6::port) == 2);
}

TEST(net_inet6_port, construction) {
  EXPECT_NE(core::net::inet6::port(1234),
            core::net::inet6::port(1234, core::net::network_byte_order_t{}));
  EXPECT_EQ(core::net::inet6::port(65535),
            core::net::inet6::port(65535, core::net::network_byte_order_t{}));
  EXPECT_EQ(core::net::inet6::port(0),
            core::net::inet6::port(0, core::net::network_byte_order_t{}));
}

}  // namespace tests::net::inet6
