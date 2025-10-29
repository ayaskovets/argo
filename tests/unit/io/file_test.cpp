#include <sandbox/io/fd.hpp>

#include <latch>
#include <thread>

#include <gtest/gtest.h>

// #include "impl.hpp"

namespace tests::io {

namespace io = sandbox::io;

TEST(io_fd, read_write) {
  const std::vector<std::byte> data{std::byte(5), std::byte(6), std::byte(7)};

  std::latch latch(2);
  const std::jthread writer([&]() {
    latch.arrive_and_wait();
    EXPECT_EQ(io::fd::kStdin().write(data), data.size());
  });

  latch.count_down();

  std::vector<std::byte> buffer{std::byte(1), std::byte(2), std::byte(3),
                                std::byte(4)};
  EXPECT_EQ(io::fd::kStdin().read(buffer), data.size());
  // EXPECT_EQ(std::span(buffer.data(), data.size()), data);
}

// TODO: test read & write etc

}  // namespace tests::io
