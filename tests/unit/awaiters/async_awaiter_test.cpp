#include <argo/awaiters/async_awaiter.hpp>
#include <argo/executors/inline_executor.hpp>

#include <flow/spsc_queue.hpp>
#include <gtest/gtest.h>

namespace tests::unit {

TEST(async_awaiter, smoke) {
  auto child = []() -> argo::task<int> { co_return 1; }();

  // NOLINTNEXTLINE(cppcoreguidelines-avoid-reference-coroutine-parameters)
  auto parent = [](argo::task<int> & child) -> argo::task<void> {
    co_await co_await argo::awaiters::async(child);
  }(child);

  flow::spsc_queue<argo::task_ref> in_channel(1);
  flow::spsc_queue<std::nullptr_t> out_channel(1);
  argo::executors::inline_executor executor(in_channel, out_channel);

  EXPECT_FALSE(parent.done());
  EXPECT_FALSE(child.done());

  executor.context().resume(argo::task_ref(parent));
  EXPECT_FALSE(parent.done());
  EXPECT_FALSE(child.done());
  EXPECT_EQ(in_channel.size(), 1);

  executor.run_once();
  EXPECT_FALSE(parent.done());
  EXPECT_TRUE(child.done());
  EXPECT_EQ(in_channel.size(), 1);

  executor.run_once();
  EXPECT_TRUE(parent.done());
  EXPECT_TRUE(in_channel.empty());
}

}  // namespace tests::unit
