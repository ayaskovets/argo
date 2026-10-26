#include <argo/scheduler.hpp>
#include <argo/schedulers/network_scheduler.hpp>
#include <argo/schedulers/timer_scheduler.hpp>

#include <flow/spsc_queue.hpp>
#include <gtest/gtest.h>
#include <skynet/base_socket.hpp>

namespace tests::unit {

TEST(scheduler, network_scheduler) {
  using scheduler_type = argo::schedulers::network_scheduler<
      flow::spsc_queue<argo::schedulers::network_in_value *>,
      flow::spsc_queue<argo::execution_token>>;

  static_assert(argo::concepts::scheduler<scheduler_type>);
}

TEST(scheduler, timer_scheduler) {
  using scheduler_type = argo::schedulers::timer_scheduler<
      std::chrono::steady_clock,
      flow::spsc_queue<
          argo::schedulers::timer_in_value<std::chrono::steady_clock> *>,
      flow::spsc_queue<argo::execution_token>>;

  static_assert(argo::concepts::scheduler<scheduler_type>);
}

}  // namespace tests::unit
