#include <sandbox/io/base_fd.hpp>

#include <gtest/gtest.h>

namespace tests::io {

TEST(io_base_fd, size) {
  static_assert(sizeof(sandbox::io::base_fd) == 4);
  static_assert(alignof(sandbox::io::base_fd) == 4);
}

TEST(io_base_fd, manual_close) {
  sandbox::io::base_fd fd(sandbox::io::base_fd::kStdin());
  EXPECT_NO_THROW(fd.close());
  EXPECT_ANY_THROW(fd.close());
  EXPECT_ANY_THROW(fd.close());
}

TEST(io_base_fd, copy_assign_to_itself) {
  sandbox::io::base_fd fd(sandbox::io::base_fd::kStdin());
  sandbox::io::base_fd& fd_ref = fd;
  fd = fd_ref;
  EXPECT_EQ(fd, fd_ref);
  EXPECT_NO_THROW(fd.close());
  EXPECT_ANY_THROW(fd.close());
}

TEST(io_base_fd, move_assign_to_itself) {
  sandbox::io::base_fd fd(sandbox::io::base_fd::kStdin());
  sandbox::io::base_fd& fd_ref = fd;
  fd = std::move(fd_ref);
  EXPECT_EQ(fd, fd_ref);
  EXPECT_NO_THROW(fd.close());
  EXPECT_ANY_THROW(fd.close());
}

TEST(io_base_fd, transfer_ownership) {
  sandbox::io::base_fd moved_from(sandbox::io::base_fd::kStdin());
  sandbox::io::base_fd moved_to = std::move(moved_from);
  EXPECT_NE(moved_from, moved_to);
  EXPECT_ANY_THROW(moved_from.close());
  EXPECT_NE(moved_from, moved_to);
  EXPECT_NO_THROW(moved_to.close());
  EXPECT_EQ(moved_from, moved_to);
}

TEST(io_base_fd, duplicate) {
  sandbox::io::base_fd original(sandbox::io::base_fd::kStdin());
  sandbox::io::base_fd copy(original);
  EXPECT_NE(original, copy);
  EXPECT_NO_THROW(original.close());
  EXPECT_NO_THROW(copy.close());
  EXPECT_EQ(original, copy);

  copy = sandbox::io::base_fd::kStdout();
  EXPECT_NE(copy, sandbox::io::base_fd::kStdout());
  EXPECT_NO_THROW(copy.close());
}

TEST(io_base_fd, equality) {
  EXPECT_EQ(sandbox::io::base_fd::kStdin(), sandbox::io::base_fd::kStdin());
  EXPECT_EQ(sandbox::io::base_fd::kStdout(), sandbox::io::base_fd::kStdout());
  EXPECT_EQ(sandbox::io::base_fd::kStderr(), sandbox::io::base_fd::kStderr());
  EXPECT_NE(sandbox::io::base_fd::kStdin(), sandbox::io::base_fd::kStdout());
  EXPECT_NE(sandbox::io::base_fd::kStdout(), sandbox::io::base_fd::kStderr());
}

}  // namespace tests::io
