#include <sandbox/io/fd.hpp>

#include <gtest/gtest.h>

namespace tests::io {

namespace io = sandbox::io;

TEST(io_fd, size) {
  static_assert(sizeof(io::fd) == 4);
  static_assert(alignof(io::fd) == 4);
}

TEST(io_fd, manual_close) {
  io::fd fd(io::fd::kStdin());
  EXPECT_NO_THROW(fd.close());
  EXPECT_ANY_THROW(fd.close());
  EXPECT_ANY_THROW(fd.close());
}

TEST(io_fd, copy_assign_to_itself) {
  io::fd fd(io::fd::kStdin());
  io::fd& fd_ref = fd;
  fd = fd_ref;
  EXPECT_EQ(fd, fd_ref);
  EXPECT_NO_THROW(fd.close());
  EXPECT_ANY_THROW(fd.close());
}

TEST(io_fd, move_assign_to_itself) {
  io::fd fd(io::fd::kStdin());
  io::fd& fd_ref = fd;
  fd = std::move(fd_ref);
  EXPECT_EQ(fd, fd_ref);
  EXPECT_NO_THROW(fd.close());
  EXPECT_ANY_THROW(fd.close());
}

TEST(io_fd, transfer_ownership) {
  io::fd moved_from(io::fd::kStdin());
  io::fd moved_to = std::move(moved_from);
  EXPECT_NE(moved_from, moved_to);
  EXPECT_ANY_THROW(moved_from.close());
  EXPECT_NE(moved_from, moved_to);
  EXPECT_NO_THROW(moved_to.close());
  EXPECT_EQ(moved_from, moved_to);
}

TEST(io_fd, duplicate) {
  io::fd original(io::fd::kStdin());
  io::fd copy(original);
  EXPECT_NE(original, copy);
  EXPECT_NO_THROW(original.close());
  EXPECT_NO_THROW(copy.close());
  EXPECT_EQ(original, copy);

  copy = io::fd::kStdout();
  EXPECT_NE(copy, io::fd::kStdout());
  EXPECT_NO_THROW(copy.close());
}

TEST(io_fd, equality) {
  EXPECT_EQ(io::fd::kStdin(), io::fd::kStdin());
  EXPECT_EQ(io::fd::kStdout(), io::fd::kStdout());
  EXPECT_EQ(io::fd::kStderr(), io::fd::kStderr());
  EXPECT_NE(io::fd::kStdin(), io::fd::kStdout());
  EXPECT_NE(io::fd::kStdout(), io::fd::kStderr());
}

TEST(io_fd, flags) {
  io::fd fd(io::fd::kStdin());

  io::fd::flags_t flags{};
  EXPECT_EQ(fd.get_flags(), flags);
  for (const auto flag :
       std::initializer_list<io::fd::flag>{io::fd::flag::kNonblock}) {
    flags |= flag;
    fd.set_flags(flags);
    EXPECT_EQ(fd.get_flags(), flags);
  }
}

}  // namespace tests::io
