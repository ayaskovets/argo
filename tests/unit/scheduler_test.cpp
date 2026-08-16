#include <argo/awaiters/timer_awaiter.hpp>
#include <argo/scheduler.hpp>
#include <argo/schedulers/timer_scheduler.hpp>

#include <flow/spsc_queue.hpp>
#include <gtest/gtest.h>

namespace tests::unit {

TEST(scheduler, timer_scheduler) {
  using scheduler_t = argo::schedulers::timer_scheduler<
      flow::spsc_queue<argo::awaiters::timer_awaiter<> const *>,
      flow::spsc_queue<argo::task_ref>>;

  static_assert(argo::concepts::scheduler<scheduler_t>);
}

}  // namespace tests::unit
