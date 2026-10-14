#pragma once

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
  using params_type     = std::pair<execution_token, time_point_type>;

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

 public:
  constexpr auto operator co_await() && noexcept -> auto {
    class timer_awaiter final {
     public:
      explicit constexpr timer_awaiter(time_point_type schedule_at) noexcept
          : schedule_at_(schedule_at) {}

     public:
      constexpr auto await_ready() const noexcept -> bool {
        return schedule_at_ == kScheduleImmediately;
      }

      /**
       * @brief Reschedule current task at the specified time point
       */
      constexpr void await_suspend(execution_token token) const {
        execution_context::this_executor().schedule(
            params_type{std::move(token), schedule_at_});
      }

      constexpr void await_resume() const noexcept {}

     private:
      time_point_type schedule_at_;
    };

    return timer_awaiter(schedule_at_);
  }

 private:
  time_point_type schedule_at_;
};

}  // namespace argo::awaiters
