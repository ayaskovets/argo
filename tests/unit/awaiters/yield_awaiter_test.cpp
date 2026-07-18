#include <argo/awaiters/yield_awaiter.hpp>
#include <argo/executors/inline_executor.hpp>
#include <argo/task.hpp>

#include <flow/spsc_queue.hpp>
#include <gtest/gtest.h>

namespace tests::unit::awaiters {
// NOLINTBEGIN(readability-magic-numbers)

TEST(yield_awaiter, smoke) {
  std::string log;

  // NOLINTNEXTLINE(cppcoreguidelines-avoid-reference-coroutine-parameters)
  auto const task = [](std::string & log) -> argo::task<void> {
    log.append("1");
    co_await argo::awaiters::yield();
    log.append("2");
    co_return;
  }(log);

  flow::spsc_queue<std::coroutine_handle<>> in_channel(2);
  flow::spsc_queue<std::coroutine_handle<>> out_channel(2);
  argo::executors::inline_executor executor(in_channel, out_channel);

  EXPECT_TRUE(log.empty());
  in_channel.push(task.handle());
  EXPECT_TRUE(log.empty());

  executor.run_once();
  EXPECT_EQ(log, "1");

  executor.run_once();
  EXPECT_EQ(log, "12");

  executor.run_once();
  EXPECT_EQ(log, "12");
}

// NOLINTEND(readability-magic-numbers)
}  // namespace tests::unit::awaiters
