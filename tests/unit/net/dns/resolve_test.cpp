#include <sandbox/net/dns.hpp>

#include <gtest/gtest.h>

namespace tests::net::dns {

TEST(net_dns_resolve, localhost_udp_ip4) {
  const auto results = sandbox::net::resolve(
      "localhost", sandbox::net::family::kInet, sandbox::net::protocol::kUdp);
  EXPECT_EQ(results.size(), 1);

  const auto& sockaddr = results.at(0);
  EXPECT_EQ(std::get<sandbox::net::inet::sockaddr>(sockaddr).get_ip(),
            sandbox::net::inet::ip("127.0.0.1"));
}

TEST(net_dns_resolve, localhost_tcp_ip6) {
  const auto results = sandbox::net::resolve(
      "localhost", sandbox::net::family::kInet6, sandbox::net::protocol::kTcp);
  EXPECT_EQ(results.size(), 1);

  const auto& sockaddr = results.at(0);
  EXPECT_EQ(std::get<sandbox::net::inet6::sockaddr>(sockaddr).get_ip(),
            sandbox::net::inet6::ip("::1"));
}

TEST(net_dns_resolve, localhost_unspec) {
  const auto results =
      sandbox::net::resolve("localhost", sandbox::net::family::kUnspecified,
                            sandbox::net::protocol::kUnspecified);
  EXPECT_EQ(results.size(), 4);
}

}  // namespace tests::net::dns
