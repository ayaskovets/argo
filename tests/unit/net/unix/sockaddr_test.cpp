#include <sandbox/net/unix/sockaddr.hpp>

#include <gtest/gtest.h>

namespace tests::net::unix {

namespace unix = sandbox::net::unix;

TEST(net_unix_sockaddr, size) {
  static_assert(sizeof(unix::sockaddr) == 16);
  static_assert(alignof(unix::sockaddr) == 8);
}

TEST(net_unix_sockaddr, construction) {
  EXPECT_NO_THROW(unix::sockaddr(""));
  EXPECT_NO_THROW(unix::sockaddr("/path/to/a/file"));
  EXPECT_ANY_THROW(unix::sockaddr(
      "/loooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo"
      "oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo"
      "oooooooooooooooooooooooooong"));
}

TEST(net_unix_sockaddr, equality) {
  EXPECT_EQ(unix::sockaddr::kEmpty(), unix::sockaddr::kEmpty());
  EXPECT_EQ(unix::sockaddr("/path/to/a/file"),
            unix::sockaddr("/path/to/a/file"));
  EXPECT_NE(unix::sockaddr("/path/to/a/file"),
            unix::sockaddr("/path/to/a/file2"));
  EXPECT_NE(unix::sockaddr("/path/to/a/file2"),
            unix::sockaddr("/path/to/a/file"));
}

TEST(net_unix_sockaddr, to_string) {
  EXPECT_EQ(unix::sockaddr("net_unix_sockaddr_to_string").to_string(),
            "net_unix_sockaddr_to_string");
}

TEST(net_unix_sockaddr, format) {
  const unix::sockaddr sockaddr("net_unix_sockaddr_to_string");
  EXPECT_EQ(std::format("{}", sockaddr), sockaddr.to_string());
}

}  // namespace tests::net::unix
