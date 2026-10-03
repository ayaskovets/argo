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
  explicit constexpr async_awaiter(task<T> & task) noexcept : task_(task) {}

 public:
  constexpr auto await_ready() const noexcept -> bool { return true; }

  constexpr void await_suspend(auto &&) const noexcept { std::unreachable(); }

  /**
   * @note Noexcept if this_executor::execute is noexcept
   */
  constexpr auto await_resume() -> task_future<T> {
    this_executor::execute(task_ref(task_));
    return task_future<T>(task_);
  }

 private:
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-const-or-ref-data-members
  task<T> & task_;
};

}  // namespace argo::awaiters

/**
 * @section Awaiter constructors
 */
namespace argo::awaiters {

template <typename T>
constexpr auto async(task<T> & task) noexcept -> async_awaiter<T> {
  return async_awaiter(task);
}

}  // namespace argo::awaiters
