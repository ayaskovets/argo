#include <sandbox/net/inet6/sockaddr.hpp>

#include <gtest/gtest.h>

namespace tests::net::inet6 {

TEST(net_inet6_sockaddr, size) {
  static_assert(sizeof(sandbox::net::inet6::sockaddr) == 16);
  static_assert(alignof(sandbox::net::inet6::sockaddr) == 8);
}

TEST(net_inet6_sockaddr, equality) {
  EXPECT_EQ(sandbox::net::inet6::sockaddr(sandbox::net::inet6::ip::kLoopback(),
                                          sandbox::net::inet6::port(1)),
            sandbox::net::inet6::sockaddr(sandbox::net::inet6::ip::kLoopback(),
                                          sandbox::net::inet6::port(1)));
  EXPECT_NE(sandbox::net::inet6::sockaddr(sandbox::net::inet6::ip::kLoopback(),
                                          sandbox::net::inet6::port(2)),
            sandbox::net::inet6::sockaddr(sandbox::net::inet6::ip::kLoopback(),
                                          sandbox::net::inet6::port(1)));
  EXPECT_NE(sandbox::net::inet6::sockaddr(sandbox::net::inet6::ip::kAny(),
                                          sandbox::net::inet6::port(1)),
            sandbox::net::inet6::sockaddr(sandbox::net::inet6::ip::kLoopback(),
                                          sandbox::net::inet6::port(1)));
}

TEST(net_inet6_sockaddr, get_parts) {
  const sandbox::net::inet6::ip ip("1e:fe56::");
  const sandbox::net::inet6::port port(1452);
  const sandbox::net::inet6::sockaddr sockaddr(ip, port);
  EXPECT_EQ(sockaddr.get_ip(), ip);
  EXPECT_EQ(sockaddr.get_port(), port);
}

TEST(net_inet6_sockaddr, to_string) {
  EXPECT_EQ(sandbox::net::inet6::sockaddr(sandbox::net::inet6::ip::kLoopback(),
                                          sandbox::net::inet6::port(65110))
                .to_string(),
            "[::1]:65110");
  EXPECT_EQ(
      sandbox::net::inet6::sockaddr(sandbox::net::inet6::ip("2001:db8::1"),
                                    sandbox::net::inet6::port(1123))
          .to_string(),
      "[2001:db8::1]:1123");
}

TEST(net_inet6_sockaddr, format) {
  const sandbox::net::inet6::sockaddr sockaddr(sandbox::net::inet6::ip::kAny(),
                                               sandbox::net::inet6::port(0));
  EXPECT_EQ(std::format("{}", sockaddr), sockaddr.to_string());
}

}  // namespace tests::net::inet6
