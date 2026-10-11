#include <argo/executors/inline_executor.hpp>
#include <argo/task.hpp>
#include <argo/task_future.hpp>

#include <flow/spsc_queue.hpp>
#include <gtest/gtest.h>

namespace tests::unit {

TEST(task_future, unfinished) {
  flow::spsc_queue<argo::execution_token> in_channel(1);
  flow::spsc_queue<std::nullptr_t> out_channel(1);
  argo::executors::inline_executor executor(in_channel, out_channel);

  auto child  = []() -> argo::task<int> { co_return 1; }();
  auto parent = [](argo::task<int> * child) -> argo::task<void> {
    co_await child->get_future();
  }(&child);

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
  auto child  = []() -> argo::task<int> { co_return 1; }();
  auto parent = [](argo::task<int> * child) -> argo::task<void> {
    co_await child->get_future();
  }(&child);

  child.resume();
  EXPECT_FALSE(parent.done());
  EXPECT_TRUE(child.done());

  parent.resume();
  EXPECT_TRUE(parent.done());
}

TEST(task_future, waitlist) {
  flow::spsc_queue<argo::execution_token> in_channel1(2);
  flow::spsc_queue<argo::execution_token> in_channel2(1);
  flow::spsc_queue<std::nullptr_t> out_channel(1);
  argo::executors::inline_executor executor1(in_channel1, out_channel);
  argo::executors::inline_executor executor2(in_channel2, out_channel);

  argo::task<int> awaitable     = []() -> argo::task<int> { co_return 1; }();
  argo::task_future<int> future = awaitable.get_future();
  auto const make_awaiter =
      [](argo::task_future<int> * future) -> argo::task<int> {
    co_return co_await *future;
  };
  argo::task<int> awaiter1 = make_awaiter(&future);
  argo::task<int> awaiter2 = make_awaiter(&future);

  in_channel1.push(awaiter1.get_execution_token());
  in_channel1.push(awaiter2.get_execution_token());
  EXPECT_FALSE(awaitable.done());
  EXPECT_EQ(in_channel1.size(), 2);

  executor1.run_once();
  executor1.run_once();
  EXPECT_FALSE(awaiter1.done());
  EXPECT_FALSE(awaiter2.done());
  EXPECT_TRUE(in_channel1.empty());

  in_channel2.push(awaitable.get_execution_token());
  EXPECT_EQ(in_channel2.size(), 1);

  executor2.run_once();
  EXPECT_TRUE(awaitable.done());
  EXPECT_EQ(awaitable.get(), 1);
  EXPECT_TRUE(in_channel2.empty());
  EXPECT_EQ(in_channel1.size(), 2);

  executor1.run_once();
  executor1.run_once();
  EXPECT_TRUE(awaiter1.done());
  EXPECT_EQ(awaiter1.get(), 1);
  EXPECT_TRUE(awaiter2.done());
  EXPECT_EQ(awaiter2.get(), 1);
  EXPECT_TRUE(in_channel2.empty());
  EXPECT_TRUE(in_channel1.empty());
}

}  // namespace tests::unit
