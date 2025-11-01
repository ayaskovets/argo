#include <sandbox/net/inet6/ip.hpp>

#include <gtest/gtest.h>

namespace tests::net::inet6 {

namespace inet6 = sandbox::net::inet6;
namespace net = sandbox::net;

TEST(net_inet6_ip, size) {
  static_assert(sizeof(inet6::ip) == 16);
  static_assert(alignof(inet6::ip) == 1);
}

TEST(net_inet6_ip, loopback) {
  EXPECT_EQ(inet6::ip("::1"), inet6::ip::kLoopback());
}

TEST(net_inet6_ip, any) { EXPECT_EQ(inet6::ip("::"), inet6::ip::kAny()); }

TEST(net_inet6_ip, construction) {
  const std::array<std::byte, 16> bytes{
      std::byte(0), std::byte(0), std::byte(0x25), std::byte(0x5e),
      std::byte(0), std::byte(0), std::byte(0),    std::byte(0),
      std::byte(0), std::byte(0), std::byte(0),    std::byte(0),
      std::byte(0), std::byte(0), std::byte(0),    std::byte(0)};
  const std::string string("0:255e::");

  const inet6::ip from_bytes(std::span<const std::byte, 16>(bytes),
                             net::network_byte_order_t{});
  const inet6::ip from_string(string);

  EXPECT_EQ(from_bytes, from_string);
  EXPECT_EQ(bytes, from_bytes.get_bytes(net::network_byte_order_t{}));
  EXPECT_EQ(from_bytes.to_string(), string);
  EXPECT_EQ(from_bytes.get_bytes(net::network_byte_order_t{}),
            from_string.get_bytes(net::network_byte_order_t{}));
  EXPECT_EQ(from_bytes.to_string(), from_string.to_string());
}

TEST(net_inet6_ip, invalid) {
  EXPECT_ANY_THROW(inet6::ip("2002::x"));
  EXPECT_ANY_THROW(inet6::ip("256.0.0.0"));
  EXPECT_ANY_THROW(inet6::ip("11132123123abc"));
}

TEST(net_inet6_ip, to_string) {
  EXPECT_EQ(inet6::ip("::f").to_string(), "::f");
  EXPECT_EQ(inet6::ip("2002:cb00:7101::").to_string(), "2002:cb00:7101::");
  EXPECT_EQ(inet6::ip("2001:db8::1").to_string(), "2001:db8::1");
}

TEST(net_inet6_ip, format) {
  const inet6::ip ip(inet6::ip::kAny());
  EXPECT_EQ(std::format("{}", ip), ip.to_string());
}

}  // namespace tests::net::inet6
