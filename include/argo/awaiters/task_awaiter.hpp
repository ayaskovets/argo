#pragma once

#include <cassert>
#include <coroutine>

namespace argo {
template <typename T>
class task;
}  // namespace argo

namespace argo::awaiters {

/**
 * @brief Utility awaiter that implements co_await for the task type
 */
template <typename T>
class task_awaiter final {
 public:
  explicit constexpr task_awaiter(task<T> && task) noexcept
      : task_(std::move(task)) {}

 public:
  constexpr auto await_ready() const noexcept -> bool { return task_.done(); }

  constexpr auto await_suspend(std::coroutine_handle<> caller) const noexcept
      -> std::coroutine_handle<> {
    assert(task_.handle_.promise().compare_exchange_continuation(caller) &&
           "task_awaiter::await_suspend(): failed to set continuation");
    return task_.handle_;
  }

  constexpr auto await_resume() -> decltype(auto) {
    return std::move(task_).get();
  }

 private:
  task<T> task_;
};

}  // namespace argo::awaiters

/**
 * @section Awaiter constructors
 * @note Placed in the same folder as task type
 */
namespace argo {

template <typename T>
constexpr auto operator co_await(task<T> && task) noexcept
    -> awaiters::task_awaiter<T> {
  return awaiters::task_awaiter(std::move(task));
}

}  // namespace argo