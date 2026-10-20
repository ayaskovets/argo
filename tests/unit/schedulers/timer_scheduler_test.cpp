#include <argo/awaiters/timer_awaiter.hpp>
#include <argo/executors/inline_executor.hpp>
#include <argo/schedulers/timer_scheduler.hpp>
#include <argo/task.hpp>

#include <flow/spsc_queue.hpp>
#include <gtest/gtest.h>
#include <swiss/chrono.hpp>

namespace tests::unit::schedulers {

TEST(timer_scheduler, smoke) {
  flow::spsc_queue<argo::execution_token> in_channel(1);
  flow::spsc_queue<
      argo::awaiters::timer<swiss::tls_mocked_clock<>>::params_type *>
      out_channel(1);
  argo::executors::inline_executor executor(in_channel, out_channel);
  argo::schedulers::timer_scheduler<swiss::tls_mocked_clock<>,
                                    decltype(out_channel), decltype(in_channel)>
      scheduler(out_channel, in_channel);

  auto task = []() -> argo::task<void> {
    co_await argo::awaiters::sleep_for<swiss::tls_mocked_clock<>>(
        std::chrono::milliseconds(1));
  }();

  in_channel.push(task.get_execution_token());
  EXPECT_FALSE(task.done());
  EXPECT_TRUE(out_channel.empty());

  executor.run_once();
  EXPECT_FALSE(task.done());
  EXPECT_TRUE(in_channel.empty());

  scheduler.run_once();
  EXPECT_FALSE(task.done());
  EXPECT_TRUE(in_channel.empty());
  EXPECT_TRUE(out_channel.empty());

  swiss::tls_mocked_clock<>::shift(std::chrono::milliseconds(1));
  scheduler.run_once();
  EXPECT_EQ(in_channel.size(), 1);
  EXPECT_TRUE(out_channel.empty());

  executor.run_once();
  EXPECT_TRUE(task.done());
  EXPECT_TRUE(in_channel.empty());
  EXPECT_TRUE(out_channel.empty());
  EXPECT_NO_THROW(std::move(task).get());
}

}  // namespace tests::unit::schedulers
