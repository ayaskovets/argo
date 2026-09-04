#pragma once

#include "../execution_context.hpp"
#include "../task_ref.hpp"

namespace argo::awaiters {

/**
 * @brief Suspend execution until some time point in future
 */
template <typename Clock = std::chrono::steady_clock>
class timer_awaiter final {
 public:
  using clock_type = Clock;

 private:
  using time_point_type = clock_type::time_point;

 private:
  /**
   * @brief A value indicating that the time point is in the past which means
   * that scheduler can be bypassed
   */
  static constexpr time_point_type kScheduleImmediately{};

 public:
  explicit constexpr timer_awaiter(time_point_type time_point) noexcept
      : schedule_at_(time_point > clock_type::now() ? time_point
                                                    : kScheduleImmediately) {}

  template <typename Duration>
  explicit constexpr timer_awaiter(Duration duration) noexcept
      : schedule_at_(duration > Duration(0) ? clock_type::now() + duration
                                            : kScheduleImmediately) {}

 public:
  constexpr auto await_ready() const noexcept -> bool {
    return schedule_at_ == kScheduleImmediately;
  }

  /**
   * @brief Reschedule current task at the specified time point
   * @note Noexcept if this_executor::schedule is noexcept
   */
  template <typename Caller>
  constexpr void await_suspend(Caller && caller) const {
    caller_ = std::forward<Caller>(caller);
    // TODO: pass caller to schedule and pass around pairs
    this_executor::schedule(this);
  }

  constexpr void await_resume() const noexcept {}

 public:
  /**
   * @brief Time point to schedule the task at
   */
  constexpr auto time_point() const noexcept -> time_point_type {
    return schedule_at_;
  }

  /**
   * @brief Task to schedule at the specified time point
   */
  constexpr auto task_ref() const noexcept -> task_ref { return caller_; }

 private:
  time_point_type schedule_at_;
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
constexpr auto sleep_for(auto duration) noexcept -> decltype(auto) {
  return timer_awaiter<Clock>(duration);
}

}  // namespace argo::awaiters
