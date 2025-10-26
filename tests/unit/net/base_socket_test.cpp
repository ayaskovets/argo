#include <sandbox/net/base_socket.hpp>

#include <gtest/gtest.h>

namespace tests::net {

namespace net = sandbox::net;

namespace {

class impl_socket final : public net::base_socket {
 public:
  impl_socket(net::family family, net::socktype type, net::protocol protocol)
      : net::base_socket(family, type, protocol) {}
};

}  // namespace

TEST(net_sockets_base_socket, size) {
  static_assert(sizeof(net::base_socket) == 4);
  static_assert(alignof(net::base_socket) == 4);
}

class net_sockets_base_socket
    : public ::testing::TestWithParam<
          std::tuple<net::family, net::socktype, net::protocol>> {};

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

  EXPECT_FALSE(socket.get_reuseaddr());
  EXPECT_FALSE(socket.get_reuseport());
  EXPECT_FALSE(socket.get_keepalive());
  socket.set_nonblock(true);
  socket.set_reuseaddr(true);
  socket.set_reuseport(true);
  socket.set_keepalive(true);
  EXPECT_TRUE(socket.get_reuseaddr());
  EXPECT_TRUE(socket.get_reuseport());
  EXPECT_TRUE(socket.get_keepalive());
  socket.set_nonblock(false);
  socket.set_reuseaddr(false);
  socket.set_reuseport(false);
  socket.set_keepalive(false);
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
        ::testing::Values<net::family>(net::family::kInet, net::family::kInet6,
                                       net::family::kUnix),
        ::testing::Values<net::socktype>(
            net::socktype::kStream, net::socktype::kDgram, net::socktype::kRaw,
            net::socktype::kRdm, net::socktype::kSeqpacket),
        ::testing::Values<net::protocol>(net::protocol::kUnspecified,
                                         net::protocol::kTcp,
                                         net::protocol::kUdp)));

}  // namespace tests::net
