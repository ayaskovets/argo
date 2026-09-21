#pragma once

#include "../execution_context.hpp"
#include "../task.hpp"
#include "../task_ref.hpp"

namespace argo::awaiters {

/**
 * @brief Utility awaiter that implements concurrent execution of a task within
 * another task
 * @warning The forked task that is destructed without being awaited is
 * considered a programming error and automatically terminates the program. For
 * background tasks use detach
 */
template <typename T>
class [[nodiscard]] fork_awaiter final {
 public:
  explicit constexpr fork_awaiter(task<T> && task) noexcept
      : task_(std::move(task)) {}

 public:
  constexpr auto await_ready() const noexcept -> bool {
    return task_.handle_.promise().done_without_continuation();
  }

  constexpr auto await_suspend(std::coroutine_handle<> caller) const noexcept
      -> std::coroutine_handle<> {
    /**
     * @note If task is done at this point, compare_exchange_continuation()
     * will fail and the suspended caller can resume immediately
     */
    if (!task_.handle_.promise().compare_exchange_continuation(caller)) {
      return caller;
    }
    return std::noop_coroutine();
  }

  constexpr auto await_resume() -> decltype(auto) {
    return std::move(task_).get();
  }

 private:
  task<T> task_;
};

template <typename T>
class fork_awaiter_wrapper final {
 public:
  explicit constexpr fork_awaiter_wrapper(task<T> && task) noexcept
      : task_(std::move(task)) {}

 public:
  constexpr auto await_ready() const noexcept -> bool { return true; }

  constexpr void await_suspend(auto &&) const noexcept { std::unreachable(); }

  /**
   * @brief Schedule concurrent execution and return a dedicated awaiter for
   * getting the result of the forked parallel operation
   * @note Noexcept if this_executor::execute is noexcept
   */
  constexpr auto await_resume() -> fork_awaiter<T> {
    this_executor::execute(task_ref(task_));
    return fork_awaiter(std::move(task_));
  }

 private:
  task<T> task_;
};

}  // namespace argo::awaiters

/**
 * @section Awaiter constructors
 */
namespace argo::awaiters {

template <typename T>
constexpr auto fork(task<T> && task) noexcept -> fork_awaiter_wrapper<T> {
  return fork_awaiter_wrapper(std::move(task));
}

}  // namespace argo::awaiters
