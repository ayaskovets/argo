#pragma once

#include <chrono>

#include "async/scheduler.hpp"

namespace core::async::schedulers {

/**
 * @brief Decorator for timed calls to run_once()
 */
template <async::concepts::scheduler Scheduler>
class timed_decorator : public Scheduler {
 public:
  template <typename Clock = std::chrono::steady_clock>
  constexpr void
  run_until(const typename Clock::time_point& time_point) noexcept(
      noexcept(Scheduler::run_once())) {
    while (Clock::now() < time_point) [[likely]] {
      Scheduler::run_once();
    }
  }

  template <typename Clock = std::chrono::steady_clock, typename Duration>
  constexpr void run_for(Duration duration) noexcept(
      noexcept(Scheduler::run_once())) [[likely]] {
    run_until(Clock::now() + duration);
  }
};

}  // namespace core::async::schedulers
