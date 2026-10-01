#pragma once

#include "../execution_context.hpp"
#include "../task.hpp"
#include "../task_future.hpp"
#include "../task_ref.hpp"

namespace argo::awaiters {

/**
 * @brief Awaiter that implements parallel execution of a task within another
 * task scope
 */
template <typename T>
class async_awaiter final {
 public:
  explicit constexpr async_awaiter(task<T> && task) noexcept
      : task_(std::move(task)) {}

 public:
  constexpr auto await_ready() const noexcept -> bool { return true; }

  constexpr void await_suspend(auto &&) const noexcept { std::unreachable(); }

  /**
   * @note Noexcept if this_executor::schedule is noexcept
   */
  constexpr auto await_resume() -> task_future<T> {
    task_ref const ref(task_);
    task_future<T> future(std::move(task_));

    this_executor::execute(ref);
    return future;
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
constexpr auto async(task<T> && task) noexcept -> async_awaiter<T> {
  return async_awaiter(std::move(task));
}

}  // namespace argo::awaiters
