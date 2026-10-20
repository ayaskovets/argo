#include <argo/awaiters/timer_awaiter.hpp>
#include <argo/executors/inline_executor.hpp>
#include <argo/task.hpp>

#include <flow/spsc_queue.hpp>
#include <gtest/gtest.h>
#include <swiss/chrono.hpp>

namespace tests::unit::awaiters {

TEST(timer_awaiter, skip_suspend) {
  auto task = []() -> argo::task<void> {
    co_await argo::awaiters::sleep_for<swiss::tls_mocked_clock<>>(
        std::chrono::milliseconds(0));
    co_await argo::awaiters::sleep_until<swiss::tls_mocked_clock<>>(
        swiss::tls_mocked_clock<>::now());
  }();

  EXPECT_FALSE(task.done());
  task.resume();
  EXPECT_TRUE(task.done());
}

TEST(timer_awaiter, suspend) {
  flow::spsc_queue<argo::execution_token> in_channel(1);
  flow::spsc_queue<
      argo::awaiters::timer<swiss::tls_mocked_clock<>>::params_type *>
      out_channel(1);
  argo::executors::inline_executor executor(in_channel, out_channel);

  auto task = []() -> argo::task<void> {
    co_await argo::awaiters::sleep_for<swiss::tls_mocked_clock<>>(
        std::chrono::milliseconds(1));
  }();

  in_channel.push(task.get_execution_token());
  EXPECT_EQ(in_channel.size(), 1);
  EXPECT_FALSE(task.done());

  executor.run_once();
  EXPECT_FALSE(task.done());
  EXPECT_EQ(out_channel.size(), 1);
  EXPECT_TRUE(out_channel.try_pop(std::ignore));

  in_channel.push(task.get_execution_token());
  EXPECT_FALSE(task.done());
  EXPECT_EQ(in_channel.size(), 1);

  executor.run_once();
  EXPECT_TRUE(task.done());
  EXPECT_NO_THROW(std::move(task).get());
}

}  // namespace tests::unit::awaiters
