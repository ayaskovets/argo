#include <argo/awaiters/network_awaiter.hpp>
#include <argo/awaiters/timer_awaiter.hpp>
#include <argo/scheduler.hpp>
#include <argo/schedulers/network_scheduler.hpp>
#include <argo/schedulers/timer_scheduler.hpp>

#include <flow/spsc_queue.hpp>
#include <gtest/gtest.h>
#include <skynet/base_socket.hpp>

namespace tests::unit {

TEST(scheduler, network_scheduler) {
  using scheduler_type = argo::schedulers::network_scheduler<
      flow::spsc_queue<argo::awaiters::network<
          skynet::base_socket, decltype([](skynet::reactor::event_filter)
                                            -> void {})>::params_type *>,
      flow::spsc_queue<argo::execution_token>>;

  static_assert(argo::concepts::scheduler<scheduler_type>);
}

TEST(scheduler, timer_scheduler) {
  using scheduler_type = argo::schedulers::timer_scheduler<
      std::chrono::steady_clock,
      flow::spsc_queue<argo::awaiters::timer<>::params_type *>,
      flow::spsc_queue<argo::execution_token>>;

  static_assert(argo::concepts::scheduler<scheduler_type>);
}

}  // namespace tests::unit
