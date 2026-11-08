#include <argo/executors/inline_executor.hpp>
#include <argo/task.hpp>

#include <flow/spsc_queue.hpp>
#include <gtest/gtest.h>

namespace tests::unit::awaiters {

TEST(inline_executor, smoke) {
  flow::spsc_queue<argo::execution_token> in_channel(1);
  flow::spsc_queue<std::nullptr_t> out_channel(1);
  argo::executors::inline_executor executor(in_channel, out_channel);

  std::size_t times_called = 0;
  auto task = [](std::size_t * times_called) -> argo::task<void> {
    *times_called += 1;
    co_return;
  }(&times_called);

  in_channel.push(task.get_execution_token());
  EXPECT_EQ(times_called, 0);

  EXPECT_TRUE(executor.run_once());
  EXPECT_EQ(times_called, 1);

  EXPECT_FALSE(executor.run_once());
  EXPECT_EQ(times_called, 1);
}

}  // namespace tests::unit::awaiters
