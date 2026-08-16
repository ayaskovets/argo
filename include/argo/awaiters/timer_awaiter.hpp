#pragma once

#include "../execution_context.hpp"
#include "../task_ref.hpp"

namespace argo::awaiters {

/**
 * @brief Suspend execution until some time point in future
 */
template <typename Clock = std::chrono::steady_clock>
class timer_awaiter final {
 private:
  /**
   * @brief A value indicating that the time point is in the past which means
   * that scheduler can be bypassed
   */
  static constexpr typename Clock::time_point kScheduleImmediately{};

 public:
  explicit constexpr timer_awaiter(
      typename Clock::time_point time_point) noexcept
      : schedule_at_(time_point > Clock::now() ? time_point
                                               : kScheduleImmediately) {}

  template <typename Duration>
  explicit constexpr timer_awaiter(Duration duration) noexcept
      : schedule_at_(duration > Duration(0) ? Clock::now() + duration
                                            : kScheduleImmediately) {}

 public:
  constexpr auto await_ready() const noexcept -> bool {
    return schedule_at_ == kScheduleImmediately;
  }

  /**
   * @brief Reschedule current task at the specified time point
   * @note Noexcept if this_executor::schedule is noexcept
   */
  constexpr void await_suspend(task_ref caller) const {
    caller_ = caller;
    this_executor::schedule(this);
  }

  constexpr void await_resume() const noexcept {}

 public:
  /**
   * @brief Time point to schedule the task at
   */
  constexpr auto time_point() const noexcept -> typename Clock::time_point {
    return schedule_at_;
  }

  /**
   * @brief Task to schedule at the specified time point
   */
  constexpr auto task_ref() const noexcept -> task_ref { return caller_; }

 private:
  typename Clock::time_point schedule_at_;
  mutable class task_ref caller_;
};

}  // namespace argo::awaiters

/**
 * @section Utility awaiter constructors
 */
namespace argo::awaiters {

template <typename Clock = std::chrono::steady_clock>
constexpr auto sleep_until(typename Clock::time_point time_point) noexcept
    -> decltype(auto) {
  return timer_awaiter<Clock>(time_point);
}

template <typename Clock = std::chrono::steady_clock>
constexpr auto sleep_for(auto && duration) noexcept -> decltype(auto) {
  return timer_awaiter<Clock>(std::forward<decltype(duration)>(duration));
}

}  // namespace argo::awaiters
