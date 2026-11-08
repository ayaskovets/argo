#include <argo/executors/inline_executor.hpp>
#include <argo/task.hpp>

#include <thread>

#include <flow/mpsc_queue.hpp>
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

  EXPECT_TRUE(executor.run_once());
  EXPECT_FALSE(parent.done());
  EXPECT_FALSE(child.done());
  EXPECT_TRUE(in_channel.empty());

  child.resume();
  EXPECT_FALSE(parent.done());
  EXPECT_TRUE(child.done());
  EXPECT_EQ(in_channel.size(), 1);

  EXPECT_TRUE(executor.run_once());
  EXPECT_TRUE(parent.done());
  EXPECT_NO_THROW(parent.get());
  EXPECT_TRUE(in_channel.empty());
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

TEST(task_future, waitlist_sync_co_await) {
  flow::spsc_queue<argo::execution_token> in_channel(2);
  flow::spsc_queue<std::nullptr_t> out_channel(1);
  argo::executors::inline_executor executor(in_channel, out_channel);

  argo::task<int> awaitable     = []() -> argo::task<int> { co_return 1; }();
  argo::task_future<int> future = awaitable.get_future();
  auto const make_awaiter =
      [](argo::task_future<int> * future) -> argo::task<int> {
    co_return co_await *future;
  };
  argo::task<int> awaiter1 = make_awaiter(&future);
  argo::task<int> awaiter2 = make_awaiter(&future);
  auto parent              = [](argo::task<int> awaitable) -> argo::task<int> {
    co_return co_await std::move(awaitable);
  }(std::move(awaitable));

  in_channel.push(awaiter1.get_execution_token());
  in_channel.push(awaiter2.get_execution_token());
  EXPECT_TRUE(executor.run_once());
  EXPECT_TRUE(executor.run_once());
  EXPECT_FALSE(awaiter1.done());
  EXPECT_FALSE(awaiter2.done());
  EXPECT_TRUE(in_channel.empty());

  in_channel.push(parent.get_execution_token());
  EXPECT_TRUE(executor.run_once());
  EXPECT_TRUE(parent.done());
  EXPECT_EQ(parent.get(), 1);
  EXPECT_EQ(in_channel.size(), 2);

  EXPECT_TRUE(executor.run_once());
  EXPECT_TRUE(executor.run_once());
  EXPECT_EQ(awaiter1.get(), 1);
  EXPECT_EQ(awaiter2.get(), 1);
}

TEST(task_future, noncopyable_type) {
  auto awaitable = []() -> argo::task<std::unique_ptr<int>> {
    co_return std::make_unique<int>(1);
  }();
  auto future_awaiter =
      [](argo::task_future<std::unique_ptr<int>> future) -> argo::task<int> {
    auto const & value = co_await future;
    co_return *value;
  }(awaitable.get_future());
  auto parent =
      [](argo::task<std::unique_ptr<int>> awaitable) -> argo::task<int> {
    auto const & value = co_await std::move(awaitable);
    co_return *value;
  }(std::move(awaitable));

  parent.resume();
  ASSERT_TRUE(parent.done());
  EXPECT_EQ(parent.get(), 1);

  future_awaiter.resume();
  ASSERT_TRUE(future_awaiter.done());
  EXPECT_EQ(future_awaiter.get(), 1);
}

TEST(task_future, cancel_before_done) {
  flow::spsc_queue<argo::execution_token> in_channel(1);
  flow::spsc_queue<std::nullptr_t> out_channel(1);
  argo::executors::inline_executor executor(in_channel, out_channel);

  bool started = false;
  auto child   = [](bool * started) -> argo::task<void> {
    *started = true;
    co_return;
  }(&started);
  auto future = child.get_future();
  auto parent = [](argo::task_future<void> * future) -> argo::task<void> {
    co_await *future;
  }(&future);

  in_channel.push(parent.get_execution_token());
  EXPECT_TRUE(executor.run_once());
  EXPECT_FALSE(parent.done());

  future.request_cancel();
  EXPECT_FALSE(child.done());
  EXPECT_TRUE(in_channel.empty());

  in_channel.push(child.get_execution_token());
  EXPECT_TRUE(executor.run_once());
  EXPECT_TRUE(child.done());
  EXPECT_FALSE(started);
  EXPECT_EQ(in_channel.size(), 1);

  EXPECT_TRUE(executor.run_once());
  EXPECT_TRUE(parent.done());
  EXPECT_THROW(parent.get(), argo::promise_cancelled);
}

TEST(task_future, cancel_after_done) {
  auto child  = []() -> argo::task<int> { co_return 1; }();
  auto future = child.get_future();
  auto const make_parent =
      [](argo::task_future<int> * future) -> argo::task<int> {
    co_return co_await *future;
  };
  auto parent1 = make_parent(&future);
  auto parent2 = make_parent(&future);

  child.resume();
  parent1.resume();
  EXPECT_EQ(parent1.get(), 1);

  future.request_cancel();
  parent2.resume();
  EXPECT_THROW(parent2.get(), argo::promise_cancelled);
}

TEST(task_future, notification_lifetime) {
  flow::spsc_queue<argo::execution_token> waiter_channel(1);
  flow::spsc_queue<argo::execution_token> producer_channel(1);
  flow::spsc_queue<std::nullptr_t> out_channel(1);
  argo::executors::inline_executor producer_executor(producer_channel,
                                                     out_channel);
  argo::executors::inline_executor waiter_executor(waiter_channel, out_channel);
  std::weak_ptr<int> lifetime;
  int result = 0;

  {
    auto value    = std::make_shared<int>(1);
    lifetime      = value;
    auto producer = [](std::shared_ptr<int> value) -> argo::task<int> {
      co_return *value;
    }(std::move(value));
    auto waiter = [](argo::task_future<int> future,
                     int * result) -> argo::task<void> {
      *result = co_await future;
    }(producer.get_future(), &result);
    waiter_channel.push(waiter.get_execution_token());
    EXPECT_TRUE(waiter_executor.run_once());
    producer_channel.push(producer.get_execution_token());
  }

  EXPECT_FALSE(lifetime.expired());
  EXPECT_TRUE(producer_executor.run_once());
  EXPECT_FALSE(lifetime.expired());
  EXPECT_TRUE(waiter_executor.run_once());
  EXPECT_TRUE(lifetime.expired());
  EXPECT_EQ(result, 1);
}

TEST(task_future, completion_race) {
  constexpr int kIterations = 500;
  flow::mpsc_queue<argo::execution_token> parent_channel(2);
  flow::spsc_queue<argo::execution_token> child_channel(1);
  flow::spsc_queue<std::nullptr_t> out_channel(1);
  argo::executors::inline_executor parent_executor(parent_channel, out_channel);
  argo::executors::inline_executor child_executor(child_channel, out_channel);

  for (int i = 0; i < kIterations; ++i) {
    auto child  = []() -> argo::task<int> { co_return 1; }();
    auto parent = [](argo::task_future<int> future) -> argo::task<int> {
      co_return co_await future;
    }(child.get_future());

    child_channel.push(child.get_execution_token());
    parent_channel.push(parent.get_execution_token());
    std::jthread thread(
        [&] -> void { EXPECT_TRUE(child_executor.run_once()); });
    EXPECT_TRUE(parent_executor.run_once());
    thread.join();
    parent_executor.run_once();
    ASSERT_TRUE(parent.done());
    EXPECT_EQ(parent.get(), 1);
  }
}

}  // namespace tests::unit
