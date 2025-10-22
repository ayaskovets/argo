#pragma once

#include <sandbox/async/executors/current_executor.hpp>
#include <sandbox/async/schedulers/timer_scheduler.hpp>
#include <sandbox/async/task.hpp>
#include <sandbox/utils/mixins.hpp>

namespace sandbox::async::awaiters {

template <typename Clock, typename T>
class timer_awaiter final : public utils::non_copyable,
                            public utils::non_movable {
 public:
  constexpr explicit timer_awaiter(typename Clock::time_point&& on,
                                   task<T>&& child) noexcept
      : params_(std::move(on), child.get_handle()), task_(std::move(child)) {}

 public:
  constexpr auto await_ready() { return false; }

  template <typename Promise>
  auto await_suspend(std::coroutine_handle<Promise> parent) {
    task_.get_handle().promise().set_parent(parent);

    executors::current_executor::suspend(params_);
  }

  constexpr auto await_resume() { return task_.get(); }

 private:
  schedulers::detail::timer_scheduler_input<Clock, std::coroutine_handle<>>
      params_;
  task<T> task_;
};

template <typename Clock, typename T>
constexpr auto on(typename Clock::time_point time_point,
                  task<T>&& task) noexcept {
  return timer_awaiter<Clock, T>(std::move(time_point), std::move(task));
}

template <typename Clock, typename Duration, typename T>
constexpr auto after(Duration duration, task<T>&& task) noexcept {
  return on<Clock, T>(Clock::now() + duration, std::move(task));
}

}  // namespace sandbox::async::awaiters
