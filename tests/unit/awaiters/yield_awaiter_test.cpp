#include <argo/awaiters/yield_awaiter.hpp>
#include <argo/executors/inline_executor.hpp>
#include <argo/task.hpp>

#include <flow/spsc_queue.hpp>
#include <gtest/gtest.h>

namespace tests::unit::awaiters {
// NOLINTBEGIN(readability-magic-numbers)

TEST(yield_awaiter, smoke) {
  int stage = 0;

  // NOLINTNEXTLINE(cppcoreguidelines-avoid-reference-coroutine-parameters)
  auto const task = [](int & stage) -> argo::task<void> {
    ++stage;
    co_await argo::awaiters::yield();
    ++stage;
    co_return;
  }(stage);

  EXPECT_EQ(stage, 0);

  flow::spsc_queue<std::coroutine_handle<>> in_channel(2);
  flow::spsc_queue<std::coroutine_handle<>> out_channel(2);
  argo::executors::inline_executor executor(in_channel, out_channel);

  EXPECT_EQ(stage, 0);

  in_channel.push(task.handle());

  EXPECT_EQ(stage, 0);

  executor.run_once();
  EXPECT_EQ(stage, 1);

  executor.run_once();
  EXPECT_EQ(stage, 2);
}

// NOLINTEND(readability-magic-numbers)
}  // namespace tests::unit::awaiters
