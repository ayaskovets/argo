#include <argo/awaiters/fork_awaiter.hpp>
#include <argo/executors/inline_executor.hpp>

#include <flow/spsc_queue.hpp>
#include <gtest/gtest.h>

namespace tests::unit {

TEST(fork_awaiter, smoke) {
  flow::spsc_queue<argo::task_ref> in_channel(1);
  flow::spsc_queue<std::nullptr_t> out_channel(1);
  argo::executors::inline_executor executor(in_channel, out_channel);

  auto child     = []() -> argo::task<int> { co_return 1; }();
  auto child_ref = argo::task_ref(child);

  auto task = [](argo::task<int> child) -> argo::task<void> {
    auto future = co_await argo::awaiters::fork(std::move(child));
    co_await std::move(future);
    co_return;
  }(std::move(child));

  EXPECT_FALSE(task.done());
  EXPECT_FALSE(child_ref.done());

  in_channel.push(argo::task_ref(task));
  executor.run_once();
  EXPECT_FALSE(task.done());
  EXPECT_FALSE(child_ref.done());
  EXPECT_EQ(in_channel.size(), 1);
  EXPECT_TRUE(out_channel.empty());

  executor.run_once();
  EXPECT_TRUE(task.done());
}

}  // namespace tests::unit
