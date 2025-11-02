#include <sandbox/async/schedulers/timer_scheduler.hpp>
#include <sandbox/queues/dereferencing_decorator.hpp>
#include <sandbox/queues/locked_std_deque.hpp>

#include <chrono>

#include <gtest/gtest.h>

namespace tests::async::schedulers {

namespace async = sandbox::async;

namespace {

template <typename T>
using channel_t = sandbox::queues::locked_std_deque<T>;

template <typename T>
using scheduler_t = async::schedulers::timer_scheduler<
    std::chrono::steady_clock,
    channel_t<async::schedulers::detail::timer_scheduler_input<
        std::chrono::steady_clock, T>>,
    channel_t<T>>;

}  // namespace

TEST(async_schedulers_timer_scheduler, size) {
  static_assert(sizeof(scheduler_t<int>) == 80);
  static_assert(alignof(scheduler_t<int>) == 8);
}

TEST(tmp, tmp) {
  sandbox::queues::dereferencing_decorator<channel_t<int*>> q(3);
  static_assert(std::is_same_v<typename decltype(q)::value_type, int&>);

  int tmp = 42;
  q.push(tmp);
  ++tmp;
  EXPECT_TRUE(q.try_push(tmp));
  EXPECT_EQ(q.pop(), 43);
  int value = 0;
  EXPECT_TRUE(q.try_pop(value));
  EXPECT_EQ(value, 43);
}

TEST(async_schedulers_timer_scheduler, channels) {
  channel_t<async::schedulers::detail::timer_scheduler_input<
      std::chrono::steady_clock, int>>
      in_channel(1);
  channel_t<int> out_channel(1);
  scheduler_t<int> scheduler(in_channel, out_channel);

  EXPECT_EQ(&scheduler.in_channel(), &in_channel);
  EXPECT_EQ(&scheduler.out_channel(), &out_channel);
}

// using in_channel_value_t =
//     async::schedulers::detail::timer_scheduler_input<
//         clock_t, std::coroutine_handle<>>;
// using out_channel_value_t = std::coroutine_handle<>;

// using timer_channel_t =
//     sandbox::queues::lockfree_mpmc_queue<const in_channel_value_t*>;
// using handle_channel_t =
// sandbox::queues::lockfree_mpmc_queue<out_channel_value_t>;

// using scheduler_t =
//     async::schedulers::timer_scheduler<clock_t, timer_channel_t,
//                                              handle_channel_t>;
// using executor_t =
//     async::executors::thread_pool_executor<handle_channel_t,
//                                                  timer_channel_t>;

// TEST(async_task, timer_smoke) {
//   std::cerr << std::chrono::system_clock::now() << " - begin test" <<
//   std::endl;

//   std::stop_source stop;

//   timer_channel_t timer_channel(128);
//   handle_channel_t handle_channel(128);
//   scheduler_t scheduler(timer_channel, handle_channel);

//   executor_t executor(handle_channel, timer_channel);
//   auto task = outer();

//   static_assert(async::concepts::executor<executor_t>);

//   const in_channel_value_t value(clock_t::now(), task.get_handle());
//   timer_channel.push(&value);

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
