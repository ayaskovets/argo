#include <sandbox/io/poller.hpp>

#include <gtest/gtest.h>

namespace tests::io {

namespace io = sandbox::io;

TEST(io_poller, size) {
  static_assert(sizeof(io::poller) == 120);
  static_assert(alignof(io::poller) == 8);
}

TEST(io_poller, move) {
  io::poller poller(io::poller::callback_t{});
  io::poller moved_to(std::move(poller));
}

TEST(io_poller, insert_or_assign_erase) {
  io::base_fd fd_stdin(io::base_fd::kStdin());
  io::base_fd fd_stdout(io::base_fd::kStdout());

  io::poller poller(io::poller::callback_t{});
  EXPECT_FALSE(poller.erase(fd_stdin));
  EXPECT_FALSE(poller.erase(fd_stdout));
  EXPECT_TRUE(poller.insert_or_assign(fd_stdin, io::poller::event::kPollIn));
  EXPECT_FALSE(poller.insert_or_assign(fd_stdin, io::poller::event::kPollIn));
  EXPECT_FALSE(poller.erase(fd_stdout));
  EXPECT_TRUE(poller.insert_or_assign(fd_stdout, io::poller::event::kPollIn));
  EXPECT_TRUE(poller.erase(fd_stdin));
  EXPECT_FALSE(poller.erase(fd_stdin));
  EXPECT_TRUE(poller.erase(fd_stdout));
  EXPECT_FALSE(poller.erase(fd_stdout));
}

// TODO: test poll

}  // namespace tests::io