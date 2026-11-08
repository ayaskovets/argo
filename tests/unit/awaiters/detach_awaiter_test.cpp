#include <argo/awaiters/detach_awaiter.hpp>
#include <argo/executors/inline_executor.hpp>

#include <flow/spsc_queue.hpp>
#include <gtest/gtest.h>

namespace tests::unit {

TEST(detach_awaiter, smoke) {
  flow::spsc_queue<argo::execution_token> in_channel(2);
  flow::spsc_queue<std::nullptr_t> out_channel(1);
  argo::executors::inline_executor executor(in_channel, out_channel);

  auto child  = []() -> argo::task<void> { co_return; }();
  auto parent = [](argo::task<void> child) -> argo::task<void> {
    co_await argo::awaiters::detach(std::move(child));
  }(std::move(child));

  in_channel.push(parent.get_execution_token());
  EXPECT_FALSE(parent.done());
  EXPECT_EQ(in_channel.size(), 1);

  EXPECT_TRUE(executor.run_once());
  EXPECT_TRUE(parent.done());
  EXPECT_EQ(in_channel.size(), 1);

  EXPECT_TRUE(executor.run_once());
  EXPECT_TRUE(in_channel.empty());
}

}  // namespace tests::unit
