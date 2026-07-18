#pragma once

#include <coroutine>

#include <swiss/mixins.hpp>

#include "../task.hpp"
#include "../this_executor.hpp"

namespace argo::awaiters {

/**
 * @brief Suspend execution until some time point in future
 */
template <typename Clock, typename T>
class timer_awaiter final : public swiss::non_copyable,
                            public swiss::non_movable {
 private:
  /**
   * @brief A value indicating that the time point is in the past which means
   * that scheduler can be bypassed
   */
  static constexpr typename Clock::time_point kScheduleNow{};

  /**
   * @brief A flag indicating that awaiter has to task attached to it and has to
   * simply reschedule the current coroutine at the specified timepoint. Task
   * with nullopt_t value is used in the utility awaiter constructors below
   */
  static constexpr bool kHasChildTask =
      !std::is_same_v<T, noop_task_value_type>;

 public:
  constexpr timer_awaiter(typename Clock::time_point time_point,
                          task<T> child) noexcept
      : params_(time_point > Clock::now() ? time_point : kScheduleNow,
                child.handle()),
        child_(std::move(child)) {}

  template <typename Duration>
  constexpr timer_awaiter(Duration duration, task<T> child) noexcept
      : params_(duration > Duration(0) ? Clock::now() + duration : kScheduleNow,
                child.handle()),
        child_(std::move(child)) {}

 public:
  constexpr auto await_ready() const noexcept -> bool {
    return std::get<0>(params_) == kScheduleNow;
  }

  /**
   * @brief Reschedule current task at the specified time point or set
   * continuation if there is a child task and schedule the child task it at the
   * specified time point
   * @note Noexcept if this_executor::schedule is noexcept
   */
  constexpr void await_suspend(std::coroutine_handle<> parent) {
    if constexpr (kHasChildTask) {
      child_.set_continuation(parent);
    } else {
      std::get<1>(params_) = parent;
    }

    this_executor::schedule(&static_cast<timer_awaiter const *>(this)->params_);
  }

  constexpr auto await_resume() -> decltype(auto) {
    return std::move(child_).get();
  }

 private:
  std::pair<typename Clock::time_point, std::coroutine_handle<>> params_;
  [[no_unique_address]] task<T> child_;
};

}  // namespace argo::awaiters

/**
 * @section Utility awaiter constructors
 */
namespace argo::awaiters {

template <typename Clock, typename T>
constexpr auto on(typename Clock::time_point time_point, task<T> child) noexcept
    -> decltype(auto) {
  return timer_awaiter<Clock, T>(time_point, std::move(child));
}

template <typename Clock>
constexpr auto sleep_until(typename Clock::time_point time_point) noexcept
    -> decltype(auto) {
  return timer_awaiter<Clock, noop_task_value_type>(time_point, noop_task());
}

template <typename Clock, typename Duration, typename T>
constexpr auto after(Duration duration, task<T> child) noexcept
    -> decltype(auto) {
  return timer_awaiter<Clock, T>(duration, std::move(child));
}

template <typename Clock, typename Duration>
constexpr auto sleep_for(Duration duration) noexcept -> decltype(auto) {
  return timer_awaiter<Clock, noop_task_value_type>(duration, noop_task());
}

}  // namespace argo::awaiters
