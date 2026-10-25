#include <argo/awaiters/async_awaiter.hpp>
#include <argo/executors/inline_executor.hpp>

#include <flow/spsc_queue.hpp>
#include <gtest/gtest.h>

namespace tests::unit {

TEST(network_awaiter, smoke) {
  flow::spsc_queue<argo::execution_token> in_channel(2);
  flow::spsc_queue<std::nullptr_t> out_channel(1);
  argo::executors::inline_executor executor(in_channel, out_channel);

  auto child  = []() -> argo::task<int> { co_return 1; }();
  auto parent = [](argo::task<int> child) -> argo::task<void> {
    auto future = co_await argo::awaiters::async(std::move(child));
    co_await future;
  }(std::move(child));

  in_channel.push(parent.get_execution_token());
  EXPECT_FALSE(parent.done());
  EXPECT_EQ(in_channel.size(), 1);

  executor.run_once();
  EXPECT_FALSE(parent.done());
  EXPECT_EQ(in_channel.size(), 1);

  executor.run_once();
  EXPECT_FALSE(parent.done());
  EXPECT_EQ(in_channel.size(), 1);

  executor.run_once();
  EXPECT_TRUE(parent.done());
  EXPECT_TRUE(in_channel.empty());
}

}  // namespace tests::unit
