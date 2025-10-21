#include "net/inet/sockaddr.hpp"

#include <gtest/gtest.h>

namespace tests::net::inet {

TEST(net_inet_sockaddr, size) {
  static_assert(sizeof(core::net::inet::sockaddr) == 16);
  static_assert(alignof(core::net::inet::sockaddr) == 8);
}

TEST(net_inet_sockaddr, equality) {
  EXPECT_EQ(core::net::inet::sockaddr(core::net::inet::ip::kLoopback(),
                                      core::net::inet::port(1)),
            core::net::inet::sockaddr(core::net::inet::ip::kLoopback(),
                                      core::net::inet::port(1)));
  EXPECT_NE(core::net::inet::sockaddr(core::net::inet::ip::kLoopback(),
                                      core::net::inet::port(2)),
            core::net::inet::sockaddr(core::net::inet::ip::kLoopback(),
                                      core::net::inet::port(1)));
  EXPECT_NE(core::net::inet::sockaddr(core::net::inet::ip::kAny(),
                                      core::net::inet::port(1)),
            core::net::inet::sockaddr(core::net::inet::ip::kLoopback(),
                                      core::net::inet::port(1)));
}

TEST(net_inet_sockaddr, get_parts) {
  const core::net::inet::ip ip("100.1.4.224");
  const core::net::inet::port port(1452);
  const core::net::inet::sockaddr sockaddr(ip, port);
  EXPECT_EQ(sockaddr.get_ip(), ip);
  EXPECT_EQ(sockaddr.get_port(), port);
}

TEST(net_inet_sockaddr, to_string) {
  EXPECT_EQ(core::net::inet::sockaddr(core::net::inet::ip::kBroadcast(),
                                      core::net::inet::port(65110))
                .to_string(),
            "255.255.255.255:65110");
  EXPECT_EQ(core::net::inet::sockaddr(core::net::inet::ip::kLoopback(),
                                      core::net::inet::port(65110))
                .to_string(),
            "127.0.0.1:65110");
  EXPECT_EQ(core::net::inet::sockaddr(core::net::inet::ip("100.1.4.224"),
                                      core::net::inet::port(1452))
                .to_string(),
            "100.1.4.224:1452");
}

TEST(net_inet_sockaddr, format) {
  const core::net::inet::sockaddr sockaddr(core::net::inet::ip::kAny(),
                                           core::net::inet::port(0));
  EXPECT_EQ(std::format("{}", sockaddr), sockaddr.to_string());
}

}  // namespace tests::net::inet
