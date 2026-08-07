#include <argo/awaiters/yield_awaiter.hpp>
#include <argo/executors/inline_executor.hpp>
#include <argo/task.hpp>

#include <flow/spsc_queue.hpp>
#include <gtest/gtest.h>

namespace tests::unit::awaiters {

TEST(yield_awaiter, smoke) {
  auto task = []() -> argo::task<void> {
    co_await argo::awaiters::yield();
    co_await argo::awaiters::yield();
    co_await argo::awaiters::yield();
    co_return;
  }();

  flow::spsc_queue<argo::task_ref> in_channel(1);
  in_channel.push(argo::task_ref(task));
  /**
   * @note Output channel is irrelevant for this test
   */
  flow::spsc_queue<argo::task_ref> out_channel(1);
  argo::executors::inline_executor executor(in_channel, out_channel);

  EXPECT_EQ(in_channel.size(), 1);
  EXPECT_FALSE(task.done());

  executor.run_once();
  EXPECT_EQ(in_channel.size(), 1);
  EXPECT_FALSE(task.done());

  executor.run_once();
  EXPECT_EQ(in_channel.size(), 1);
  EXPECT_FALSE(task.done());

  executor.run_once();
  EXPECT_EQ(in_channel.size(), 1);
  EXPECT_FALSE(task.done());

  executor.run_once();
  EXPECT_TRUE(in_channel.empty());
  EXPECT_TRUE(task.done());
}

}  // namespace tests::unit::awaiters
