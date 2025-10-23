#include <sandbox/async/awaiters/timer_awaiter.hpp>
#include <sandbox/async/executor.hpp>
#include <sandbox/async/executors/inline_executor.hpp>
#include <sandbox/async/executors/thread_pool_executor.hpp>
#include <sandbox/async/schedulers/timer_scheduler.hpp>
#include <sandbox/async/task.hpp>
#include <sandbox/queues/lockfree_mpmc_queue.hpp>

#include <gtest/gtest.h>

using namespace std::chrono_literals;

namespace tests::async {

namespace async = sandbox::async;

using clock_t = std::chrono::steady_clock;

using in_channel_value_t =
    async::schedulers::detail::timer_scheduler_input<clock_t,
                                                     std::coroutine_handle<>>;
using out_channel_value_t = std::coroutine_handle<>;

using timer_channel_t =
    sandbox::queues::lockfree_mpmc_queue<const in_channel_value_t*>;
using handle_channel_t =
    sandbox::queues::lockfree_mpmc_queue<out_channel_value_t>;

using scheduler_t = async::schedulers::timer_scheduler<clock_t, timer_channel_t,
                                                       handle_channel_t>;
using executor_t =
    async::executors::thread_pool_executor<handle_channel_t, timer_channel_t>;

async::task<float> innermost() {
  std::cerr << std::chrono::system_clock::now() << " - innermost() enter"
            << std::endl;
  co_return 1;
}

async::task<int> inner() {
  std::cerr << std::chrono::system_clock::now() << " - inner() enter"
            << std::endl;

  auto val1 = co_await async::awaiters::after<clock_t>(500ms, innermost());
  std::cerr << std::chrono::system_clock::now()
            << " - inner() after 1st co_await()" << std::endl;
  auto val2 = co_await async::awaiters::on<clock_t>(clock_t::now() + 500ms,
                                                    innermost());
  std::cerr << std::chrono::system_clock::now()
            << " - inner() after 2st co_await()" << std::endl;
  co_return val1 + val2;
}

async::task<int> outer() {
  std::cerr << std::chrono::system_clock::now() << " - outer() enter"
            << std::endl;
  auto val = co_await async::awaiters::after<clock_t>(500ms, inner());
  std::cerr << std::chrono::system_clock::now()
            << " - outer() after co_await () " << std::endl;
  co_return val;
}

TEST(async_task, timer_smoke) {
  std::cerr << std::chrono::system_clock::now() << " - begin test" << std::endl;

  std::stop_source stop;

  timer_channel_t timer_channel(128);
  handle_channel_t handle_channel(128);
  scheduler_t scheduler(timer_channel, handle_channel);

  executor_t executor(handle_channel, timer_channel);
  auto task = outer();

  static_assert(async::concepts::executor<executor_t>);

  const in_channel_value_t value(clock_t::now(), task.get_handle());
  timer_channel.push(&value);

  executor.run_detached(stop.get_token());

  const auto done_at = std::chrono::steady_clock::now() + 1600ms;
  while (std::chrono::steady_clock::now() < done_at) {
    scheduler.run_once();
    // executor.run_once();
  }
  stop.request_stop();

  std::cerr << std::chrono::system_clock::now() << " - end test" << std::endl;
}

}  // namespace tests::async
