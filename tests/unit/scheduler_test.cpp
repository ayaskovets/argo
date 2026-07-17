#include <argo/awaiters/timer_awaiter.hpp>
#include <argo/scheduler.hpp>
#include <argo/schedulers/timer_scheduler.hpp>

#include <chrono>
#include <coroutine>

#include <flow/mpmc_queue.hpp>
#include <gtest/gtest.h>

namespace tests::unit {

TEST(scheduler, timer_scheduler) {
  using scheduler_t = argo::schedulers::timer_scheduler<
      flow::mpmc_queue<std::pair<std::chrono::steady_clock,
                                 std::coroutine_handle<>> const *>,
      flow::mpmc_queue<std::coroutine_handle<>>>;

  static_assert(argo::concepts::scheduler<scheduler_t>);
}

}  // namespace tests::unit
