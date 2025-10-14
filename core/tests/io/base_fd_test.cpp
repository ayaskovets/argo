#include "io/base_fd.hpp"

#include <gtest/gtest.h>

namespace tests::io {

TEST(io_base_fd, size) {
  static_assert(sizeof(core::io::base_fd) == 4);
  static_assert(alignof(core::io::base_fd) == 4);
}

TEST(io_base_fd, manual_close) {
  core::io::base_fd base_fd(core::io::base_fd::kStdin());
  EXPECT_NO_THROW(base_fd.close());
  EXPECT_ANY_THROW(base_fd.close());
  EXPECT_ANY_THROW(base_fd.close());
}

TEST(io_base_fd, assign_to_itself) {
  core::io::base_fd base_fd(core::io::base_fd::kStdin());
  core::io::base_fd& fd_ref = base_fd;
  base_fd = fd_ref;
  EXPECT_NO_THROW(base_fd.close());
  EXPECT_ANY_THROW(base_fd.close());
}

TEST(io_base_fd, transfer_ownership) {
  core::io::base_fd moved_from(core::io::base_fd::kStdin());
  core::io::base_fd moved_to = std::move(moved_from);
  EXPECT_NE(moved_from, moved_to);
  EXPECT_ANY_THROW(moved_from.close());
  EXPECT_NE(moved_from, moved_to);
  EXPECT_NO_THROW(moved_to.close());
  EXPECT_EQ(moved_from, moved_to);
}

TEST(io_base_fd, duplicate) {
  core::io::base_fd original(core::io::base_fd::kStdin());
  core::io::base_fd copy(original);
  EXPECT_NE(original, copy);
  EXPECT_NO_THROW(original.close());
  EXPECT_NO_THROW(copy.close());
  EXPECT_EQ(original, copy);
}

TEST(io_base_fd, equality) {
  EXPECT_EQ(core::io::base_fd::kStdin(), core::io::base_fd::kStdin());
  EXPECT_EQ(core::io::base_fd::kStdout(), core::io::base_fd::kStdout());
  EXPECT_EQ(core::io::base_fd::kStderr(), core::io::base_fd::kStderr());
  EXPECT_NE(core::io::base_fd::kStdin(), core::io::base_fd::kStdout());
  EXPECT_NE(core::io::base_fd::kStdout(), core::io::base_fd::kStderr());
}

}  // namespace tests::io
