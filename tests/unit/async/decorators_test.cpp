#include <wip/async/awaiters/io_awaiter.hpp>
#include <wip/async/channels/multiplexed_channel.hpp>
#include <wip/async/executors/inline_executor.hpp>
#include <wip/async/executors/thread_pool_executor.hpp>
#include <wip/async/scheduler.hpp>
#include <wip/async/schedulers/io_scheduler.hpp>
#include <wip/async/schedulers/parallel_combinator.hpp>
#include <wip/async/schedulers/sequential_combinator.hpp>
#include <wip/async/schedulers/timer_scheduler.hpp>
#include <wip/async/task.hpp>
#include <wip/net/inet/sockaddr.hpp>
#include <wip/net/inet/tcp/socket.hpp>
#include <wip/queues/lockfree_mpmc_queue.hpp>

#include <gtest/gtest.h>

using namespace std::chrono_literals;

namespace tests::async {

namespace async = wip::async;

using clock_t = std::chrono::steady_clock;
using handle_channel_t =
    wip::queues::lockfree_mpmc_queue<std::coroutine_handle<>>;

using io_in_channel_value_t =
    async::schedulers::detail::io_scheduler_input<std::coroutine_handle<>>;
using io_out_channel_value_t = std::coroutine_handle<>;
using io_channel_t =
    wip::queues::lockfree_mpmc_queue<const io_in_channel_value_t*>;
using io_scheduler_t =
    async::schedulers::io_scheduler<io_channel_t, handle_channel_t>;

using timer_in_channel_value_t =
    async::schedulers::detail::timer_scheduler_input<clock_t,
                                                     std::coroutine_handle<>>;
using timer_out_channel_value_t = std::coroutine_handle<>;
using timer_channel_t =
    wip::queues::lockfree_mpmc_queue<const timer_in_channel_value_t*>;
using timer_scheduler_t =
    async::schedulers::timer_scheduler<clock_t, timer_channel_t,
                                       handle_channel_t>;

TEST(async_task, decorators_smoke) {
  io_channel_t io_channel(128);
  handle_channel_t io_handle_channel(128);
  io_scheduler_t io_scheduler(io_channel, io_handle_channel);

  timer_channel_t timer_channel(128);
  handle_channel_t timer_handle_channel(128);
  timer_scheduler_t timer_scheduler(timer_channel, timer_handle_channel);

  static_assert(async::concepts::scheduler<io_scheduler_t>);
  static_assert(async::concepts::scheduler<timer_scheduler_t>);

  //   async::schedulers::sequential_combinator<io_scheduler_t,
  //                                                  timer_scheduler_t>
  //       decorated(std::move(io_scheduler), std::move(timer_scheduler));

  async::schedulers::sequential_combinator<io_scheduler_t, timer_scheduler_t>
      decorated(io_scheduler_t(io_channel, io_handle_channel),
                std::move(timer_scheduler));

  //   async::executors::inline_executor executor(
  //       decorated.out_channel(), decorated.in_channel());

  decorated.run_once();
  //   executor.run_once();

  async::channels::multiplexed_channel<io_channel_t&, timer_channel_t&>
      multiplexed_channel(io_channel, timer_channel);

  const async::schedulers::detail::io_scheduler_input<std::coroutine_handle<>>*
      io_ptr;
  const async::schedulers::detail::timer_scheduler_input<
      clock_t, std::coroutine_handle<>>* timer_ptr;

  wip::net::inet::tcp::socket fd(wip::io::uninitialized_t{});
  async::schedulers::detail::io_scheduler_input<std::coroutine_handle<>>
      io_params(fd, {}, nullptr);
  async::schedulers::detail::timer_scheduler_input<clock_t,
                                                   std::coroutine_handle<>>
      timer_params({}, nullptr);

  EXPECT_FALSE(multiplexed_channel.try_pop(io_ptr));
  EXPECT_FALSE(multiplexed_channel.try_pop(timer_ptr));

  EXPECT_TRUE(multiplexed_channel.try_push(&io_params));
  EXPECT_TRUE(multiplexed_channel.try_push(&timer_params));

  EXPECT_TRUE(multiplexed_channel.try_pop(io_ptr));
  EXPECT_TRUE(multiplexed_channel.try_pop(timer_ptr));
  EXPECT_FALSE(multiplexed_channel.try_pop(io_ptr));
  EXPECT_FALSE(multiplexed_channel.try_pop(timer_ptr));
}

}  // namespace tests::async
