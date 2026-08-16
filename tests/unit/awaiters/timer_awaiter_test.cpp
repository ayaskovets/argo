#include <argo/awaiters/timer_awaiter.hpp>
#include <argo/executors/inline_executor.hpp>
#include <argo/task.hpp>

#include <flow/spsc_queue.hpp>
#include <gtest/gtest.h>
#include <swiss/chrono.hpp>

namespace tests::unit::awaiters {

TEST(timer_awaiter, skip_suspend) {
  auto task = []() -> argo::task<void> {
    co_await argo::awaiters::sleep_for<swiss::mocked_clock<>>(
        std::chrono::milliseconds(0));
    co_await argo::awaiters::sleep_until<swiss::mocked_clock<>>(
        swiss::mocked_clock<>::now());
    co_return;
  }();

  EXPECT_FALSE(task.done());
  task.resume();
  EXPECT_TRUE(task.done());
}

TEST(timer_awaiter, suspend) {
  flow::spsc_queue<argo::task_ref> in_channel(1);
  flow::spsc_queue<argo::awaiters::timer_awaiter<swiss::mocked_clock<>> const *>
      out_channel(1);
  argo::executors::inline_executor executor(in_channel, out_channel);

  auto task = []() -> argo::task<void> {
    co_await argo::awaiters::sleep_for<swiss::mocked_clock<>>(
        std::chrono::milliseconds(1));
    co_return;
  }();

  in_channel.push(argo::task_ref(task));
  executor.run_once();
  EXPECT_FALSE(task.done());
  EXPECT_EQ(out_channel.size(), 1);
  EXPECT_TRUE(out_channel.try_pop(std::ignore));

  task.resume();
  EXPECT_TRUE(task.done());
  EXPECT_NO_THROW(std::move(task).get());
}

TEST(timer_awaiter, overflow) {
  // TODO: channel is full
}

}  // namespace tests::unit::awaiters
