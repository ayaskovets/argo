#pragma once

#include <swiss/concepts.hpp>
#include <swiss/scope_fail.hpp>

#include "../execution_context.hpp"
#include "../execution_token.hpp"
#include "../schedulers/timer_in_value.hpp"

namespace argo::awaiters {

/**
 * @brief Suspend execution until some time point in future
 */
template <typename Clock = std::chrono::steady_clock>
class timer final {
 private:
  using time_point_type = Clock::time_point;

  /**
   * @brief A value indicating that the time point is in the past which means
   * that scheduler can be bypassed
   */
  static constexpr time_point_type kScheduleImmediately{};

 public:
  explicit constexpr timer(time_point_type time_point) noexcept
      : schedule_at_(time_point > Clock::now() ? time_point
                                               : kScheduleImmediately) {}

  template <typename Duration>
  explicit constexpr timer(Duration duration) noexcept
      : schedule_at_(duration > Duration(0) ? Clock::now() + duration
                                            : kScheduleImmediately) {}

 private:
  class timer_awaiter final {
   public:
    explicit constexpr timer_awaiter(time_point_type schedule_at) noexcept
        : in_value_({}, schedule_at) {}

   public:
    constexpr auto await_ready() noexcept -> bool {
      return in_value_.get_time_point() == kScheduleImmediately;
    }

    /**
     * @brief Reschedule current task at the specified time point
     */
    constexpr void await_suspend(execution_token token) {
      in_value_ = {std::move(token), in_value_.get_time_point()};

      swiss::scope_fail const scope_fail([&]() noexcept -> void {
        in_value_.get_execution_token().release();
      });

      execution_context::this_executor().schedule(&in_value_);
    }

    constexpr void await_resume() const noexcept {}

   private:
    schedulers::timer_in_value<Clock> in_value_;
  };

 public:
  constexpr auto operator co_await() && noexcept -> auto {
    return timer_awaiter(schedule_at_);
  }

 private:
  time_point_type schedule_at_;
};

}  // namespace argo::awaiters

/**
 * @section Utility constructors
 */
namespace argo::awaiters {

template <typename Clock = std::chrono::steady_clock>
constexpr auto sleep_until(typename Clock::time_point time_point) noexcept
    -> timer<Clock> {
  return timer<Clock>(time_point);
}

template <typename Clock = std::chrono::steady_clock>
constexpr auto sleep_for(
    swiss::not_same_as<typename Clock::time_point> auto duration) noexcept
    -> timer<Clock> {
  return timer<Clock>(duration);
}

}  // namespace argo::awaiters
