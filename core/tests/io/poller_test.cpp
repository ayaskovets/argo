#include "io/poller.hpp"

#include <gtest/gtest.h>

namespace tests::io {

TEST(io_poller, size) {
  static_assert(sizeof(core::io::poller) == 120);
  static_assert(alignof(core::io::poller) == 8);
}

TEST(io_poller, move) {
  core::io::poller poller(core::io::poller::callback_t{});
  core::io::poller moved_to(std::move(poller));
}

TEST(io_poller, insert_or_assign_erase) {
  core::io::base_fd fd_stdin(core::io::base_fd::kStdin());
  core::io::base_fd fd_stdout(core::io::base_fd::kStdout());

  core::io::poller poller(core::io::poller::callback_t{});
  EXPECT_FALSE(poller.erase(fd_stdin));
  EXPECT_FALSE(poller.erase(fd_stdout));
  EXPECT_TRUE(
      poller.insert_or_assign(fd_stdin, core::io::poller::event::kPollIn));
  EXPECT_FALSE(
      poller.insert_or_assign(fd_stdin, core::io::poller::event::kPollIn));
  EXPECT_FALSE(poller.erase(fd_stdout));
  EXPECT_TRUE(
      poller.insert_or_assign(fd_stdout, core::io::poller::event::kPollIn));
  EXPECT_TRUE(poller.erase(fd_stdin));
  EXPECT_FALSE(poller.erase(fd_stdin));
  EXPECT_TRUE(poller.erase(fd_stdout));
  EXPECT_FALSE(poller.erase(fd_stdout));
}

// TODO: test poll

}  // namespace tests::io