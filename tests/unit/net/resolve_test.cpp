#include <sandbox/net/dns.hpp>

#include <gtest/gtest.h>

namespace tests::net {

namespace net = sandbox::net;

TEST(net_dns_resolve, localhost_udp_ip4) {
  const auto results =
      net::resolve("localhost", net::family::kInet, net::protocol::kUdp);
  EXPECT_EQ(results.size(), 1);

  const auto& sockaddr = results.at(0);
  EXPECT_EQ(std::get<net::inet::sockaddr>(sockaddr).get_ip(),
            net::inet::ip("127.0.0.1"));
}

TEST(net_dns_resolve, localhost_tcp_ip6) {
  const auto results =
      net::resolve("localhost", net::family::kInet6, net::protocol::kTcp);
  EXPECT_EQ(results.size(), 1);

  const auto& sockaddr = results.at(0);
  EXPECT_EQ(std::get<net::inet6::sockaddr>(sockaddr).get_ip(),
            net::inet6::ip("::1"));
}

TEST(net_dns_resolve, localhost_unspec) {
  const auto results = net::resolve("localhost", net::family::kUnspecified,
                                    net::protocol::kUnspecified);
  EXPECT_EQ(results.size(), 4);
}

}  // namespace tests::net
