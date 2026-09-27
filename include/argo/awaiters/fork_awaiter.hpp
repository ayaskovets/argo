#pragma once

#include "../execution_context.hpp"
#include "../task.hpp"
#include "../task_future.hpp"
#include "../task_ref.hpp"

namespace argo::awaiters {

/**
 * @brief Awaiter that implements
 */
template <typename T>
class fork_awaiter final {
 public:
  explicit constexpr fork_awaiter(task<T> && task) noexcept
      : task_(std::move(task)) {}

 public:
  constexpr auto await_ready() const noexcept -> bool { return true; }

  constexpr void await_suspend(auto &&) const noexcept { std::unreachable(); }

  /**
   * @note Noexcept if this_executor::schedule is noexcept
   */
  constexpr auto await_resume() -> task_future<T> {
    this_executor::execute(task_ref(task_));
    return task_future(std::move(task_));
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
constexpr auto fork(task<T> && task) noexcept -> fork_awaiter<T> {
  return fork_awaiter(std::move(task));
}

}  // namespace argo::awaiters
