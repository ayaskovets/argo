#include <argo/executors/inline_executor.hpp>
#include <argo/task.hpp>
#include <argo/task_future.hpp>

#include <flow/spsc_queue.hpp>
#include <gtest/gtest.h>

namespace tests::unit {

TEST(task_future, waitlist) {
  flow::spsc_queue<argo::execution_token> in_channel(1);
  flow::spsc_queue<std::nullptr_t> out_channel(1);
  argo::executors::inline_executor executor(in_channel, out_channel);

  auto child = []() -> argo::task<int> { co_return 1; }();
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-reference-coroutine-parameters)
  auto parent = [](argo::task<int> & child) -> argo::task<void> {
    co_await child.get_future();
  }(child);

  in_channel.push(parent.get_execution_token());
  EXPECT_FALSE(parent.done());
  EXPECT_FALSE(child.done());
  EXPECT_EQ(in_channel.size(), 1);

  executor.run_once();
  EXPECT_FALSE(parent.done());
  EXPECT_FALSE(child.done());
  EXPECT_TRUE(in_channel.empty());

  child.resume();
  EXPECT_FALSE(parent.done());
  EXPECT_TRUE(child.done());
  EXPECT_EQ(in_channel.size(), 1);

  executor.run_once();
  EXPECT_TRUE(parent.done());
  EXPECT_TRUE(in_channel.empty());
}

TEST(task_future, finished) {
  auto child = []() -> argo::task<int> { co_return 1; }();
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-reference-coroutine-parameters)
  auto parent = [](argo::task<int> & child) -> argo::task<void> {
    co_await child.get_future();
  }(child);

  child.resume();
  EXPECT_FALSE(parent.done());
  EXPECT_TRUE(child.done());

  parent.resume();
  EXPECT_TRUE(parent.done());
}

}  // namespace tests::unit
