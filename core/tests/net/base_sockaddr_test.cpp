#include "net/base_sockaddr.hpp"

#include <gtest/gtest.h>

namespace tests::net {

namespace {

class impl_sockaddr final : public core::net::base_sockaddr {
 public:
  explicit impl_sockaddr(core::net::family family)
      : core::net::base_sockaddr(family) {}

 public:
  using core::net::base_sockaddr::get_storage;
};

}  // namespace

TEST(net_sockets_base_sockaddr, size) {
  static_assert(sizeof(core::net::base_sockaddr) == 16);
  static_assert(alignof(core::net::base_sockaddr) == 8);
}

class net_sockets_base_sockaddr
    : public ::testing::TestWithParam<core::net::family> {};

TEST_P(net_sockets_base_sockaddr, smoke) {
  const auto& family = GetParam();
  const impl_sockaddr sockaddr(family);

  EXPECT_GT(sockaddr.get_length(), 0);
  EXPECT_EQ(sockaddr.get_family(), family);
  EXPECT_TRUE(sockaddr.get_storage());
  EXPECT_NO_THROW(sockaddr.to_string());
}

TEST_P(net_sockets_base_sockaddr, copy) {
  const auto& family = GetParam();
  const impl_sockaddr original(family);
  const impl_sockaddr copy(original);

  EXPECT_EQ(original.get_family(), copy.get_family());
  EXPECT_EQ(original.get_length(), copy.get_length());
  EXPECT_EQ(original.get_storage(), copy.get_storage());
  EXPECT_EQ(original, copy);
}

TEST_P(net_sockets_base_sockaddr, move) {
  const auto& family = GetParam();
  impl_sockaddr original(family);
  const impl_sockaddr move(std::move(original));

  EXPECT_GT(move.get_length(), 0);
  EXPECT_EQ(move.get_family(), family);
  EXPECT_TRUE(move.get_storage());
  EXPECT_NO_THROW(move.to_string());
}

INSTANTIATE_TEST_SUITE_P(net_sockets_base_sockaddr, net_sockets_base_sockaddr,
                         ::testing::Values(core::net::family::kInet,
                                           core::net::family::kInet6,
                                           core::net::family::kUnix));

}  // namespace tests::net
