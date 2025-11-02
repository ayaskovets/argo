#include <sandbox/net/inet/sockaddr.hpp>

#include <gtest/gtest.h>

namespace tests::net::inet {

namespace inet = sandbox::net::inet;

TEST(net_inet_sockaddr, size) {
  static_assert(sizeof(inet::sockaddr) == 24);
  static_assert(alignof(inet::sockaddr) == 8);
}

TEST(net_inet_sockaddr, equality) {
  EXPECT_EQ(inet::sockaddr(inet::ip::kLoopback(), inet::port(1)),
            inet::sockaddr(inet::ip::kLoopback(), inet::port(1)));
  EXPECT_NE(inet::sockaddr(inet::ip::kLoopback(), inet::port(2)),
            inet::sockaddr(inet::ip::kLoopback(), inet::port(1)));
  EXPECT_NE(inet::sockaddr(inet::ip::kAny(), inet::port(1)),
            inet::sockaddr(inet::ip::kLoopback(), inet::port(1)));
}

TEST(net_inet_sockaddr, get_parts) {
  const inet::ip ip("100.1.4.224");
  const inet::port port(1452);
  const inet::sockaddr sockaddr(ip, port);
  EXPECT_EQ(sockaddr.get_ip(), ip);
  EXPECT_EQ(sockaddr.get_port(), port);
}

TEST(net_inet_sockaddr, to_string) {
  EXPECT_EQ(
      inet::sockaddr(inet::ip::kBroadcast(), inet::port(65110)).to_string(),
      "255.255.255.255:65110");
  EXPECT_EQ(
      inet::sockaddr(inet::ip::kLoopback(), inet::port(65110)).to_string(),
      "127.0.0.1:65110");
  EXPECT_EQ(
      inet::sockaddr(inet::ip("100.1.4.224"), inet::port(1452)).to_string(),
      "100.1.4.224:1452");
}

TEST(net_inet_sockaddr, format) {
  const inet::sockaddr sockaddr(inet::ip::kAny(), inet::port(0));
  EXPECT_EQ(std::format("{}", sockaddr), sockaddr.to_string());
}

}  // namespace tests::net::inet
