#include <argo/awaiters/switch_to.hpp>
#include <argo/awaiters/yield_awaiter.hpp>
#include <argo/executors/inline_executor.hpp>
#include <argo/task.hpp>

#include <flow/spsc_queue.hpp>
#include <gtest/gtest.h>

namespace tests::unit::awaiters {

TEST(switch_to_awaiter, smoke) {
  flow::spsc_queue<argo::task_ref> in_channel1(1);
  flow::spsc_queue<argo::task_ref> in_channel2(1);
  flow::spsc_queue<std::nullptr_t> out_channel(1);
  argo::executors::inline_executor executor1(in_channel1, out_channel);
  argo::executors::inline_executor executor2(in_channel2, out_channel);

  // NOLINTNEXTLINE(cppcoreguidelines-avoid-reference-coroutine-parameters)
  auto task = [](argo::execution_context & ctx2) -> argo::task<void> {
    co_await argo::awaiters::switch_to(ctx2);
    co_await argo::awaiters::yield();
    co_return;
  }(executor2.context());

  in_channel1.push(argo::task_ref(task));
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
