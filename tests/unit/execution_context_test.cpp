#include <argo/awaiters/yield_awaiter.hpp>
#include <argo/execution_context.hpp>
#include <argo/executors/inline_executor.hpp>
#include <argo/task.hpp>

#include <flow/spsc_queue.hpp>
#include <gtest/gtest.h>

namespace tests::unit::awaiters {

TEST(execution_context, awaiter) {
  flow::spsc_queue<argo::execution_token> in_channel1(1);
  flow::spsc_queue<argo::execution_token> in_channel2(1);
  flow::spsc_queue<std::nullptr_t> out_channel(1);
  argo::executors::inline_executor executor1(in_channel1, out_channel);
  argo::executors::inline_executor executor2(in_channel2, out_channel);

  // NOLINTNEXTLINE(cppcoreguidelines-avoid-reference-coroutine-parameters)
  auto task = [](argo::execution_context const & ctx2) -> argo::task<void> {
    co_await ctx2;
    co_await argo::awaiters::yield();
  }(executor2.context());

  in_channel1.push(task.suspend());
  EXPECT_FALSE(in_channel1.empty());
  EXPECT_TRUE(in_channel2.empty());

  executor1.run_once();
  EXPECT_TRUE(in_channel1.empty());
  EXPECT_FALSE(in_channel2.empty());

  executor2.run_once();
  EXPECT_TRUE(in_channel1.empty());
  EXPECT_TRUE(in_channel2.empty());
}

}  // namespace tests::unit::awaiters
