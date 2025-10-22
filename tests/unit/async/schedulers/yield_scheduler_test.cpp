#include <sandbox/async/schedulers/yield_scheduler.hpp>
#include <sandbox/queues/locked_std_deque.hpp>

#include <gtest/gtest.h>

namespace tests::async::schedulers {

namespace {

template <typename T>
using channel_t = sandbox::queues::locked_std_deque<T>;

template <typename T>
using scheduler_t =
    sandbox::async::schedulers::yield_scheduler<channel_t<T>, channel_t<T>>;

}  // namespace

TEST(async_schedulers_yield_scheduler, size) {
  static_assert(sizeof(scheduler_t<int>) == 72);
  static_assert(alignof(scheduler_t<int>) == 8);
}

TEST(async_schedulers_yield_scheduler, channels) {
  channel_t<int> in_channel(1);
  channel_t<int> out_channel(1);
  scheduler_t<int> scheduler(in_channel, out_channel);

  EXPECT_EQ(&scheduler.in_channel(), &in_channel);
  EXPECT_EQ(&scheduler.out_channel(), &out_channel);
}

// using in_channel_value_t =
//     sandbox::async::schedulers::detail::yield_scheduler_input<
//         clock_t, std::coroutine_handle<>>;
// using out_channel_value_t = std::coroutine_handle<>;

// using yield_channel_t =
//     sandbox::queues::lockfree_mpmc_queue<const in_channel_value_t*>;
// using handle_channel_t =
// sandbox::queues::lockfree_mpmc_queue<out_channel_value_t>;

// using scheduler_t =
//     sandbox::async::schedulers::yield_scheduler<clock_t, yield_channel_t,
//                                              handle_channel_t>;
// using executor_t =
//     sandbox::async::executors::thread_pool_executor<handle_channel_t,
//                                                  yield_channel_t>;

// TEST(async_task, yield_smoke) {
//   std::cerr << std::chrono::system_clock::now() << " - begin test" <<
//   std::endl;

//   std::stop_source stop;

//   yield_channel_t yield_channel(128);
//   handle_channel_t handle_channel(128);
//   scheduler_t scheduler(yield_channel, handle_channel);

//   executor_t executor(handle_channel, yield_channel);
//   auto task = outer();

//   static_assert(sandbox::async::concepts::executor<executor_t>);

//   const in_channel_value_t value(clock_t::now(), task.get_handle());
//   yield_channel.push(&value);

//   executor.run_detached(stop.get_token());

//   const auto done_at = std::chrono::steady_clock::now() + 1600ms;
//   while (std::chrono::steady_clock::now() < done_at) {
//     scheduler.run_once();
//     // executor.run_once();
//   }
//   stop.request_stop();

//   std::cerr << std::chrono::system_clock::now() << " - end test" <<
//   std::endl;
// }

}  // namespace tests::async::schedulers
