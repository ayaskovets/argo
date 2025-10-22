#pragma once

#include <sandbox/async/scheduler.hpp>

namespace sandbox::async::schedulers {

/**
 * @brief Decorator for conditional calls and loops with run_once()
 */
template <async::concepts::scheduler Scheduler>
class conditional_decorator : public Scheduler {
 public:
  template <typename Predicate>
  constexpr void run_while(Predicate&& predicate) noexcept(
      noexcept(Scheduler::run_once())) {
    while (predicate()) [[likely]] {
      Scheduler::run_once();
    }
  }

  template <typename Predicate>
  constexpr void run_if(Predicate&& predicate) noexcept(
      noexcept(Scheduler::run_once())) {
    if (predicate()) {
      Scheduler::run_once();
    }
  }
};

}  // namespace sandbox::async::schedulers
