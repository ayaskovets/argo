#pragma once

#include "async/scheduler.hpp"

namespace core::async::schedulers {

/**
 * @brief Decorator for timed calls to run_once()
 */
template <async::concepts::scheduler Scheduler>
class timed_decorator : public Scheduler {
 public:
  template <typename Clock>
  constexpr void run_until(typename Clock::time_point&& time_point) noexcept(
      noexcept(Scheduler::run_once())) {
    while (Clock::now() < time_point) [[likely]] {
      Scheduler::run_once();
    }
  }

  template <typename Clock, typename Duration>
  constexpr void run_for(Duration duration) noexcept(
      noexcept(Scheduler::run_once())) [[likely]] {
    run_until(Clock::now() + duration);
  }
};

}  // namespace core::async::schedulers
