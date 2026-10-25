#pragma once

#include <swiss/concepts.hpp>

#include "../execution_context.hpp"
#include "../execution_token.hpp"

namespace argo::awaiters {

/**
 * @brief Suspend execution until some time point in future
 */
template <typename Clock = std::chrono::steady_clock>
class timer final {
 public:
  using clock_type      = Clock;
  using time_point_type = clock_type::time_point;
  class params_type final {
   public:
    constexpr auto execution_token() noexcept -> execution_token && {
      return std::move(execution_token_);
    }

    constexpr auto time_point() const noexcept -> time_point_type {
      return time_point_;
    }

   private:
    class execution_token execution_token_;
    time_point_type time_point_;

   private:
    /**
     * @note Not to return reference to pointer from socket()
     */
    friend class timer;
  };

 private:
  /**
   * @brief A value indicating that the time point is in the past which means
   * that scheduler can be bypassed
   */
  static constexpr time_point_type kScheduleImmediately{};

 public:
  explicit constexpr timer(time_point_type time_point) noexcept
      : schedule_at_(time_point > clock_type::now() ? time_point
                                                    : kScheduleImmediately) {}

  template <typename Duration>
  explicit constexpr timer(Duration duration) noexcept
      : schedule_at_(duration > Duration(0) ? clock_type::now() + duration
                                            : kScheduleImmediately) {}

 private:
  class timer_awaiter final {
   public:
    explicit constexpr timer_awaiter(time_point_type schedule_at) noexcept {
      params_.time_point_ = schedule_at;
    }

   public:
    constexpr auto await_ready() noexcept -> bool {
      return params_.time_point() == kScheduleImmediately;
    }

    /**
     * @brief Reschedule current task at the specified time point
     */
    constexpr void await_suspend(execution_token token) {
      params_.execution_token_ = std::move(token);
      execution_context::this_executor().schedule(&params_);
    }

    constexpr void await_resume() const noexcept {}

   private:
    params_type params_;
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
static constexpr auto sleep_until(
    typename Clock::time_point time_point) noexcept -> timer<Clock> {
  return timer<Clock>(time_point);
}

template <typename Clock = std::chrono::steady_clock>
static constexpr auto sleep_for(
    swiss::not_same_as<typename Clock::time_point> auto duration) noexcept
    -> timer<Clock> {
  return timer<Clock>(duration);
}

}  // namespace argo::awaiters
