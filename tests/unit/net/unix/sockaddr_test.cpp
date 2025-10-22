#include <sandbox/net/unix/sockaddr.hpp>

#include <gtest/gtest.h>

namespace tests::net::unix {

TEST(net_unix_sockaddr, size) {
  static_assert(sizeof(sandbox::net::unix::sockaddr) == 16);
  static_assert(alignof(sandbox::net::unix::sockaddr) == 8);
}

TEST(net_unix_sockaddr, construction) {
  EXPECT_NO_THROW(sandbox::net::unix::sockaddr(""));
  EXPECT_NO_THROW(sandbox::net::unix::sockaddr("/path/to/a/file"));
  EXPECT_ANY_THROW(sandbox::net::unix::sockaddr(
      "/loooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo"
      "oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo"
      "oooooooooooooooooooooooooong"));
}

TEST(net_unix_sockaddr, equality) {
  EXPECT_EQ(sandbox::net::unix::sockaddr::kEmpty(),
            sandbox::net::unix::sockaddr::kEmpty());
  EXPECT_EQ(sandbox::net::unix::sockaddr("/path/to/a/file"),
            sandbox::net::unix::sockaddr("/path/to/a/file"));
  EXPECT_NE(sandbox::net::unix::sockaddr("/path/to/a/file"),
            sandbox::net::unix::sockaddr("/path/to/a/file2"));
  EXPECT_NE(sandbox::net::unix::sockaddr("/path/to/a/file2"),
            sandbox::net::unix::sockaddr("/path/to/a/file"));
}

TEST(net_unix_sockaddr, to_string) {
  EXPECT_EQ(
      sandbox::net::unix::sockaddr("net_unix_sockaddr_to_string").to_string(),
      "net_unix_sockaddr_to_string");
}

TEST(net_unix_sockaddr, format) {
  const sandbox::net::unix::sockaddr sockaddr("net_unix_sockaddr_to_string");
  EXPECT_EQ(std::format("{}", sockaddr), sockaddr.to_string());
}

}  // namespace tests::net::unix
