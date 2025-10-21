#include "net/base_socket.hpp"

#include <gtest/gtest.h>

namespace tests::net {

namespace {

class impl_socket final : public core::net::base_socket {
 public:
  impl_socket(core::net::family family, core::net::socktype type,
              core::net::protocol protocol)
      : core::net::base_socket(family, type, protocol) {}
};

}  // namespace

TEST(net_sockets_base_socket, size) {
  static_assert(sizeof(core::net::base_socket) == 4);
  static_assert(alignof(core::net::base_socket) == 4);
}

class net_sockets_base_socket
    : public ::testing::TestWithParam<std::tuple<
          core::net::family, core::net::socktype, core::net::protocol>> {};

TEST_P(net_sockets_base_socket, options) {
  const auto& [family, type, protocol] = GetParam();
  try {
    impl_socket(family, type, protocol);
  } catch (...) {
    GTEST_SKIP() << "unsupported socket instance";
  }

  impl_socket socket(family, type, protocol);
  EXPECT_EQ(socket.get_family(), family);
  EXPECT_EQ(socket.get_type(), type);

  EXPECT_FALSE(socket.get_nonblock());
  EXPECT_FALSE(socket.get_reuseaddr());
  EXPECT_FALSE(socket.get_reuseport());
  EXPECT_FALSE(socket.get_keepalive());
  socket.set_nonblock(true);
  socket.set_reuseaddr(true);
  socket.set_reuseport(true);
  socket.set_keepalive(true);
  EXPECT_TRUE(socket.get_nonblock());
  EXPECT_TRUE(socket.get_reuseaddr());
  EXPECT_TRUE(socket.get_reuseport());
  EXPECT_TRUE(socket.get_keepalive());
  socket.set_nonblock(false);
  socket.set_reuseaddr(false);
  socket.set_reuseport(false);
  socket.set_keepalive(false);
  EXPECT_FALSE(socket.get_nonblock());
  EXPECT_FALSE(socket.get_reuseaddr());
  EXPECT_FALSE(socket.get_reuseport());
  EXPECT_FALSE(socket.get_keepalive());
}

TEST_P(net_sockets_base_socket, copy) {
  const auto& [family, type, protocol] = GetParam();
  try {
    impl_socket(family, type, protocol);
  } catch (...) {
    GTEST_SKIP() << "unsupported socket instance";
  }

  impl_socket original(family, type, protocol);
  impl_socket copy(original);
  EXPECT_EQ(original, original);
  EXPECT_EQ(copy, copy);
  EXPECT_NE(original, copy);
  EXPECT_NO_THROW(original.close());
  EXPECT_NO_THROW(copy.close());
}

TEST_P(net_sockets_base_socket, move) {
  const auto& [family, type, protocol] = GetParam();
  try {
    impl_socket(family, type, protocol);
  } catch (...) {
    GTEST_SKIP() << "unsupported socket instance";
  }

  impl_socket original(family, type, protocol);
  impl_socket move(std::move(original));
  EXPECT_NO_THROW(move.close());
}

INSTANTIATE_TEST_SUITE_P(
    net_sockets_base_socket, net_sockets_base_socket,
    ::testing::Combine(
        ::testing::Values<core::net::family>(core::net::family::kInet,
                                             core::net::family::kInet6,
                                             core::net::family::kUnix),
        ::testing::Values<core::net::socktype>(core::net::socktype::kStream,
                                               core::net::socktype::kDgram,
                                               core::net::socktype::kRaw,
                                               core::net::socktype::kRdm,
                                               core::net::socktype::kSeqpacket),
        ::testing::Values<core::net::protocol>(
            core::net::protocol::kUnspecified, core::net::protocol::kTcp,
            core::net::protocol::kUdp)));

}  // namespace tests::net
