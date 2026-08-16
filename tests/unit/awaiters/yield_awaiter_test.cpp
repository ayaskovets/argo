#include <argo/awaiters/yield_awaiter.hpp>
#include <argo/executors/inline_executor.hpp>
#include <argo/task.hpp>

#include <cstddef>

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
  flow::spsc_queue<std::nullptr_t> out_channel(1);
  argo::executors::inline_executor executor(in_channel, out_channel);

  in_channel.push(argo::task_ref(task));
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

TEST(yield_awaiter, overflow) {
  // TODO: overflow in push
}

TEST(yield_awaiter, exception) {
  // TODO: overflow in push
}

}  // namespace tests::unit::awaiters
