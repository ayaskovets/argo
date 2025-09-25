#include "net/inet/port.hpp"

#include <gtest/gtest.h>

namespace tests::net::inet {

TEST(net_inet_port, size) {
  static_assert(sizeof(core::net::inet::port) == 2);
  static_assert(alignof(core::net::inet::port) == 2);
}

TEST(net_inet_port, construction) {
  EXPECT_NE(core::net::inet::port(1234),
            core::net::inet::port(1234, core::net::network_byte_order_t{}));
  EXPECT_EQ(core::net::inet::port(65535),
            core::net::inet::port(65535, core::net::network_byte_order_t{}));
  EXPECT_EQ(core::net::inet::port(0),
            core::net::inet::port(0, core::net::network_byte_order_t{}));
}

}  // namespace tests::net::inet
