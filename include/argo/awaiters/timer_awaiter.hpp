#pragma once

#include <argo/task.hpp>
#include <argo/this_executor.hpp>

#include <swiss/mixins.hpp>

namespace argo::awaiters {

/**
 * @brief Suspend execution until some time point in future
 */
template <typename Clock, typename T>
class timer_awaiter final : public swiss::non_copyable,
                            public swiss::non_movable {
 private:
  static constexpr std::optional<typename Clock::time_point> kScheduleNow =
      std::nullopt;

 public:
  constexpr timer_awaiter(typename Clock::time_point time_point,
                          task<T> && child) noexcept
      : schedule_on_(time_point > Clock::now() ? time_point : kScheduleNow),
        child_(std::move(child)) {}

  template <typename Duration>
  constexpr timer_awaiter(Duration duration, task<T> && child) noexcept
      : schedule_on_(duration > static_cast<Duration>(0)
                         ? Clock::now() + duration
                         : kScheduleNow),
        child_(std::move(child)) {}

 public:
  [[nodiscard]] constexpr auto await_ready() const noexcept -> bool {
    return schedule_on_ == kScheduleNow;
  }

  constexpr void await_suspend(
      std::coroutine_handle<> const parent) const noexcept {
    // TODO: rework this method

    child_.get_handle().promise().set_parent(parent);
    this_executor::suspend(*schedule_on_, child_.get_handle());
  }

  constexpr auto await_resume() const -> T { return child_.get(); }

 private:
  std::optional<typename Clock::time_point> schedule_on_;
  task<T> child_;
};

template <typename Clock, typename T>
constexpr auto on(typename Clock::time_point time_point,
                  task<T> && child) noexcept {
  return timer_awaiter<Clock, T>(time_point, std::move(child));
}

template <typename Clock, typename Duration, typename T>
constexpr auto after(Duration duration, task<T> && child) noexcept {
  return timer_awaiter<Clock, T>(duration, std::move(child));
}

}  // namespace argo::awaiters
