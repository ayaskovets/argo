#include <argo/executors/inline_executor.hpp>
#include <argo/task_future.hpp>

#include <flow/spsc_queue.hpp>
#include <gtest/gtest.h>

namespace tests::unit {

TEST(task_future, unfinished) {
  flow::spsc_queue<argo::task_ref> in_channel(1);
  flow::spsc_queue<std::nullptr_t> out_channel(1);
  argo::executors::inline_executor executor(in_channel, out_channel);

  auto child     = []() -> argo::task<int> { co_return 1; }();
  auto child_ref = argo::task_ref(child);

  auto task = [](argo::task<int> child) -> argo::task<void> {
    co_await argo::task_future(child);
    co_return;
  }(std::move(child));

  EXPECT_FALSE(task.done());
  EXPECT_FALSE(child_ref.done());

  executor.context().resume(argo::task_ref(task));
  EXPECT_FALSE(task.done());
  EXPECT_FALSE(child_ref.done());

  child_ref.resume();
  EXPECT_FALSE(task.done());
  EXPECT_TRUE(child_ref.done());
  EXPECT_EQ(in_channel.size(), 1);

  executor.run_once();
  EXPECT_TRUE(task.done());
  EXPECT_TRUE(in_channel.empty());
}

TEST(task_future, finished) {
  auto child     = []() -> argo::task<int> { co_return 1; }();
  auto child_ref = argo::task_ref(child);

  auto task = [](argo::task<int> child) -> argo::task<void> {
    co_await argo::task_future(child);
    co_return;
  }(std::move(child));

  EXPECT_FALSE(task.done());
  EXPECT_FALSE(child_ref.done());

  child_ref.resume();
  EXPECT_FALSE(task.done());
  EXPECT_TRUE(child_ref.done());

  task.resume();
  EXPECT_TRUE(task.done());
}

}  // namespace tests::unit
