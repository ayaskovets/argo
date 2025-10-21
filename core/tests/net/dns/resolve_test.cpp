#include <gtest/gtest.h>

#include "net/dns.hpp"

namespace tests::net::dns {

TEST(net_dns_resolve, localhost_udp_ip4) {
  const auto results = core::net::resolve("localhost", core::net::family::kInet,
                                          core::net::protocol::kUdp);
  EXPECT_EQ(results.size(), 1);

  const auto& sockaddr = results.at(0);
  EXPECT_EQ(std::get<core::net::inet::sockaddr>(sockaddr).get_ip(),
            core::net::inet::ip("127.0.0.1"));
}

TEST(net_dns_resolve, localhost_tcp_ip6) {
  const auto results = core::net::resolve(
      "localhost", core::net::family::kInet6, core::net::protocol::kTcp);
  EXPECT_EQ(results.size(), 1);

  const auto& sockaddr = results.at(0);
  EXPECT_EQ(std::get<core::net::inet6::sockaddr>(sockaddr).get_ip(),
            core::net::inet6::ip("::1"));
}

TEST(net_dns_resolve, localhost_unspec) {
  const auto results =
      core::net::resolve("localhost", core::net::family::kUnspecified,
                         core::net::protocol::kUnspecified);
  EXPECT_EQ(results.size(), 4);
}

}  // namespace tests::net::dns
