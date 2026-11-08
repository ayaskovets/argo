#include <argo/awaiters/yield_awaiter.hpp>
#include <argo/executors/inline_executor.hpp>
#include <argo/task.hpp>

#include <flow/spsc_queue.hpp>
#include <gtest/gtest.h>

namespace tests::unit::awaiters {

TEST(yield_awaiter, smoke) {
  flow::spsc_queue<argo::execution_token> in_channel(1);
  flow::spsc_queue<std::nullptr_t> out_channel(1);
  argo::executors::inline_executor executor(in_channel, out_channel);

  auto task = []() -> argo::task<void> {
    co_await argo::awaiters::yield();
    co_await argo::awaiters::yield();
    co_await argo::awaiters::yield();
  }();

  in_channel.push(task.get_execution_token());
  EXPECT_EQ(in_channel.size(), 1);
  EXPECT_FALSE(task.done());

  EXPECT_TRUE(executor.run_once());
  EXPECT_EQ(in_channel.size(), 1);
  EXPECT_FALSE(task.done());

  EXPECT_TRUE(executor.run_once());
  EXPECT_EQ(in_channel.size(), 1);
  EXPECT_FALSE(task.done());

  EXPECT_TRUE(executor.run_once());
  EXPECT_EQ(in_channel.size(), 1);
  EXPECT_FALSE(task.done());

  EXPECT_TRUE(executor.run_once());
  EXPECT_TRUE(in_channel.empty());
  EXPECT_TRUE(task.done());
}

}  // namespace tests::unit::awaiters
