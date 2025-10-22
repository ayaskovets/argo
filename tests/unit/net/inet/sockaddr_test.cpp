#include <sandbox/net/inet/sockaddr.hpp>

#include <gtest/gtest.h>

namespace tests::net::inet {

TEST(net_inet_sockaddr, size) {
  static_assert(sizeof(sandbox::net::inet::sockaddr) == 16);
  static_assert(alignof(sandbox::net::inet::sockaddr) == 8);
}

TEST(net_inet_sockaddr, equality) {
  EXPECT_EQ(sandbox::net::inet::sockaddr(sandbox::net::inet::ip::kLoopback(),
                                         sandbox::net::inet::port(1)),
            sandbox::net::inet::sockaddr(sandbox::net::inet::ip::kLoopback(),
                                         sandbox::net::inet::port(1)));
  EXPECT_NE(sandbox::net::inet::sockaddr(sandbox::net::inet::ip::kLoopback(),
                                         sandbox::net::inet::port(2)),
            sandbox::net::inet::sockaddr(sandbox::net::inet::ip::kLoopback(),
                                         sandbox::net::inet::port(1)));
  EXPECT_NE(sandbox::net::inet::sockaddr(sandbox::net::inet::ip::kAny(),
                                         sandbox::net::inet::port(1)),
            sandbox::net::inet::sockaddr(sandbox::net::inet::ip::kLoopback(),
                                         sandbox::net::inet::port(1)));
}

TEST(net_inet_sockaddr, get_parts) {
  const sandbox::net::inet::ip ip("100.1.4.224");
  const sandbox::net::inet::port port(1452);
  const sandbox::net::inet::sockaddr sockaddr(ip, port);
  EXPECT_EQ(sockaddr.get_ip(), ip);
  EXPECT_EQ(sockaddr.get_port(), port);
}

TEST(net_inet_sockaddr, to_string) {
  EXPECT_EQ(sandbox::net::inet::sockaddr(sandbox::net::inet::ip::kBroadcast(),
                                         sandbox::net::inet::port(65110))
                .to_string(),
            "255.255.255.255:65110");
  EXPECT_EQ(sandbox::net::inet::sockaddr(sandbox::net::inet::ip::kLoopback(),
                                         sandbox::net::inet::port(65110))
                .to_string(),
            "127.0.0.1:65110");
  EXPECT_EQ(sandbox::net::inet::sockaddr(sandbox::net::inet::ip("100.1.4.224"),
                                         sandbox::net::inet::port(1452))
                .to_string(),
            "100.1.4.224:1452");
}

TEST(net_inet_sockaddr, format) {
  const sandbox::net::inet::sockaddr sockaddr(sandbox::net::inet::ip::kAny(),
                                              sandbox::net::inet::port(0));
  EXPECT_EQ(std::format("{}", sockaddr), sockaddr.to_string());
}

}  // namespace tests::net::inet
