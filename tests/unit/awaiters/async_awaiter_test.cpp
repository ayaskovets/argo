#include <argo/awaiters/async_awaiter.hpp>
#include <argo/executors/inline_executor.hpp>

#include <flow/spsc_queue.hpp>
#include <gtest/gtest.h>

namespace tests::unit {

TEST(async_awaiter, smoke) {
  flow::spsc_queue<argo::task_ref> in_channel(1);
  flow::spsc_queue<std::nullptr_t> out_channel(1);
  argo::executors::inline_executor executor(in_channel, out_channel);

  auto child           = []() -> argo::task<int> { co_return 1; }();
  auto const child_ref = argo::task_ref(child);

  auto task = [](argo::task<int> child) -> argo::task<void> {
    auto future = co_await argo::awaiters::async(child);
    co_await future;
    co_return;
  }(std::move(child));

  EXPECT_FALSE(task.done());
  EXPECT_FALSE(child_ref.done());

  executor.context().resume(argo::task_ref(task));
  EXPECT_FALSE(task.done());
  EXPECT_FALSE(child_ref.done());
  EXPECT_EQ(in_channel.size(), 1);

  executor.run_once();
  EXPECT_FALSE(task.done());
  EXPECT_TRUE(child_ref.done());
  EXPECT_EQ(in_channel.size(), 1);

  executor.run_once();
  EXPECT_TRUE(task.done());
  EXPECT_TRUE(in_channel.empty());
}

}  // namespace tests::unit
