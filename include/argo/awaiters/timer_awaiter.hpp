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
 public:
  using clock_type = Clock;
  using task_type  = std::coroutine_handle<>;

 public:
  constexpr timer_awaiter(typename Clock::time_point time_point,
                          task<T> child) noexcept
      : schedule_at_(time_point > Clock::now()
                         ? time_point
                         : std::optional<typename Clock::time_point>()),
        child_(std::move(child)) {}

  template <typename Duration>
  constexpr timer_awaiter(Duration duration, task<T> child) noexcept
      : schedule_at_(duration > static_cast<Duration>(0)
                         ? Clock::now() + duration
                         : std::optional<typename Clock::time_point>()),
        child_(std::move(child)) {}

 public:
  constexpr auto schedule_at() const noexcept -> typename Clock::time_point {
    return *schedule_at_;
  }

  constexpr auto schedule_task() const noexcept -> std::coroutine_handle<> {
    return child_.handle();
  }

 public:
  constexpr auto await_ready() const noexcept -> bool {
    return !schedule_at_.has_value();
  }

  constexpr void await_suspend(
      std::coroutine_handle<> const parent) const noexcept {
    child_.set_continuation(parent);
    this_executor::schedule(*schedule_at_, child_.handle());
  }

  constexpr auto await_resume() const -> T { return child_.get(); }

 private:
  std::optional<typename Clock::time_point> schedule_at_;
  task<T> child_;
};

template <typename Clock, typename T>
constexpr auto on(typename Clock::time_point time_point,
                  task<T> child) noexcept {
  return timer_awaiter<Clock, T>(time_point, std::move(child));
}

template <typename Clock, typename Duration, typename T>
constexpr auto after(Duration duration, task<T> child) noexcept {
  return timer_awaiter<Clock, T>(duration, std::move(child));
}

}  // namespace argo::awaiters
