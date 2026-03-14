#include <wip/net/inet/ip.hpp>

#include <gtest/gtest.h>

namespace tests::net::inet {

namespace inet = wip::net::inet;
namespace net = wip::net;

TEST(net_inet_ip, size) {
  static_assert(sizeof(inet::ip) == 4);
  static_assert(alignof(inet::ip) == 4);
}

TEST(net_inet_ip, loopback) {
  EXPECT_EQ(inet::ip("127.0.0.1"), inet::ip::kLoopback());
}

TEST(net_inet_ip, any) { EXPECT_EQ(inet::ip("0.0.0.0"), inet::ip::kAny()); }

TEST(net_inet_ip, broadcast) {
  EXPECT_EQ(inet::ip("255.255.255.255"), inet::ip::kBroadcast());
}

TEST(net_inet_ip, construction) {
  const std::uint32_t bytes{0xFFFFFF00};
  const std::string string("255.255.255.0");

  const inet::ip from_bytes(bytes);
  const inet::ip from_string(string);

  EXPECT_EQ(from_bytes, from_string);
  EXPECT_EQ(bytes, from_bytes.get_bytes());
  EXPECT_EQ(from_bytes.to_string(), string);
  EXPECT_EQ(from_bytes.get_bytes(), from_string.get_bytes());
  EXPECT_EQ(from_bytes.to_string(), from_string.to_string());
}

TEST(net_inet_ip, network_order) {
  const std::uint32_t bytes{0xFFFFFF00};

  const inet::ip host_byte_order(bytes);
  const inet::ip network_byte_order(bytes, net::network_byte_order_t{});

  EXPECT_EQ(host_byte_order.get_bytes(), bytes);
  EXPECT_NE(host_byte_order.get_bytes(net::network_byte_order_t{}), bytes);
  EXPECT_NE(network_byte_order.get_bytes(), bytes);
  EXPECT_EQ(network_byte_order.get_bytes(net::network_byte_order_t{}), bytes);
}

TEST(net_inet_ip, invalid) {
  EXPECT_ANY_THROW(inet::ip(""));
  EXPECT_ANY_THROW(inet::ip("256.0.0.0"));
  EXPECT_ANY_THROW(inet::ip("11132123123abc"));
}

TEST(net_inet_ip, to_string) {
  EXPECT_EQ(inet::ip("255.255.255.0").to_string(), "255.255.255.0");
  EXPECT_EQ(inet::ip("255.255.255.0").to_string(), "255.255.255.0");
}

TEST(net_inet_ip, format) {
  const inet::ip ip(inet::ip::kAny());
  EXPECT_EQ(std::format("{}", ip), ip.to_string());
}

}  // namespace tests::net::inet
