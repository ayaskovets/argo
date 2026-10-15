#include <argo/awaiters/detach_awaiter.hpp>
#include <argo/awaiters/yield_awaiter.hpp>
#include <argo/execution_context.hpp>
#include <argo/executors/inline_executor.hpp>
#include <argo/task.hpp>

#include <thread>

#include <flow/spsc_queue.hpp>
#include <gtest/gtest.h>

namespace tests::unit::awaiters {

TEST(execution_context, awaiter) {
  flow::spsc_queue<argo::execution_token> in_channel1(1);
  flow::spsc_queue<argo::execution_token> in_channel2(1);
  flow::spsc_queue<std::nullptr_t> out_channel(1);
  argo::executors::inline_executor executor1(in_channel1, out_channel);
  argo::executors::inline_executor executor2(in_channel2, out_channel);

  auto task = [](argo::execution_context const * ctx2) -> argo::task<void> {
    co_await *ctx2;
    co_await argo::awaiters::yield();
  }(&executor2.context());

  in_channel1.push(task.get_execution_token());
  EXPECT_FALSE(in_channel1.empty());
  EXPECT_TRUE(in_channel2.empty());

  executor1.run_once();
  EXPECT_TRUE(in_channel1.empty());
  EXPECT_FALSE(in_channel2.empty());

  executor2.run_once();
  EXPECT_TRUE(in_channel1.empty());
  EXPECT_TRUE(in_channel2.empty());
}

TEST(execution_context, unblock_on_destruction) {
  flow::spsc_queue<argo::execution_token> in_channel(2);
  flow::spsc_queue<std::nullptr_t> out_channel(1);
  auto executor =
      std::make_unique<argo::executors::inline_executor<decltype(in_channel),
                                                        decltype(out_channel)>>(
          in_channel, out_channel);

  auto task = []() -> argo::task<void> {
    co_await argo::awaiters::detach([]() -> argo::task<void> { co_return; }());
    co_await argo::awaiters::detach([]() -> argo::task<void> { co_return; }());
    co_await argo::awaiters::detach([]() -> argo::task<void> { co_return; }());
  }();

  in_channel.push(task.get_execution_token());
  EXPECT_EQ(in_channel.size(), 1);

  std::jthread thread([&]() -> void {
    executor->run_once();
    EXPECT_EQ(in_channel.size(), 2);
  });

  while (in_channel.size() != 2) {
  }
  executor.reset();
  thread.join();
}

}  // namespace tests::unit::awaiters
