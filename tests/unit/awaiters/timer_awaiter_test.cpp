#include <argo/awaiters/timer_awaiter.hpp>
#include <argo/awaiters/yield_awaiter.hpp>
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
      argo::schedulers::timer_in_value<swiss::tls_mocked_clock<>> *>
      out_channel(1);
  argo::executors::inline_executor executor(in_channel, out_channel);
  argo::schedulers::timer_in_value<swiss::tls_mocked_clock<>> * in_value{};

  auto task = []() -> argo::task<void> {
    co_await argo::awaiters::sleep_for<swiss::tls_mocked_clock<>>(
        std::chrono::milliseconds(1));
  }();

  in_channel.push(task.get_execution_token());
  EXPECT_EQ(in_channel.size(), 1);
  EXPECT_FALSE(task.done());

  EXPECT_TRUE(executor.run_once());
  EXPECT_FALSE(task.done());
  EXPECT_EQ(out_channel.size(), 1);
  ASSERT_TRUE(out_channel.try_pop(in_value));

  in_channel.push(in_value->get_execution_token());
  EXPECT_FALSE(task.done());
  EXPECT_EQ(in_channel.size(), 1);

  EXPECT_TRUE(executor.run_once());
  EXPECT_TRUE(task.done());
  EXPECT_NO_THROW(std::move(task).get());
}

TEST(timer_awaiter, incompatible_out_channel_type) {
  flow::spsc_queue<argo::execution_token> in_channel(1);
  flow::spsc_queue<std::nullptr_t> out_channel(1);
  argo::executors::inline_executor executor(in_channel, out_channel);

  bool caught = false;
  auto task   = [](bool * caught) -> argo::task<void> {
    try {
      co_await argo::awaiters::sleep_for<swiss::tls_mocked_clock<>>(
          std::chrono::milliseconds(1));
    } catch (std::runtime_error const &) {
      *caught = true;
    }

    co_await argo::awaiters::yield();
  }(&caught);

  in_channel.push(task.get_execution_token());
  EXPECT_TRUE(executor.run_once());
  EXPECT_TRUE(caught);
  EXPECT_FALSE(task.done());

  EXPECT_TRUE(executor.run_once());
  EXPECT_TRUE(task.done());
  EXPECT_NO_THROW(task.get());
}

}  // namespace tests::unit::awaiters
