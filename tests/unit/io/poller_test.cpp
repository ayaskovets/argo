#include <sandbox/io/poller.hpp>

#include <gtest/gtest.h>

namespace tests::io {

TEST(io_poller, size) {
  static_assert(sizeof(sandbox::io::poller) == 120);
  static_assert(alignof(sandbox::io::poller) == 8);
}

TEST(io_poller, move) {
  sandbox::io::poller poller(sandbox::io::poller::callback_t{});
  sandbox::io::poller moved_to(std::move(poller));
}

TEST(io_poller, insert_or_assign_erase) {
  sandbox::io::base_fd fd_stdin(sandbox::io::base_fd::kStdin());
  sandbox::io::base_fd fd_stdout(sandbox::io::base_fd::kStdout());

  sandbox::io::poller poller(sandbox::io::poller::callback_t{});
  EXPECT_FALSE(poller.erase(fd_stdin));
  EXPECT_FALSE(poller.erase(fd_stdout));
  EXPECT_TRUE(
      poller.insert_or_assign(fd_stdin, sandbox::io::poller::event::kPollIn));
  EXPECT_FALSE(
      poller.insert_or_assign(fd_stdin, sandbox::io::poller::event::kPollIn));
  EXPECT_FALSE(poller.erase(fd_stdout));
  EXPECT_TRUE(
      poller.insert_or_assign(fd_stdout, sandbox::io::poller::event::kPollIn));
  EXPECT_TRUE(poller.erase(fd_stdin));
  EXPECT_FALSE(poller.erase(fd_stdin));
  EXPECT_TRUE(poller.erase(fd_stdout));
  EXPECT_FALSE(poller.erase(fd_stdout));
}

// TODO: test poll

}  // namespace tests::io