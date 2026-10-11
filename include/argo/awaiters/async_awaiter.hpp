#pragma once

#include "../execution_context.hpp"
#include "../task.hpp"
#include "../task_future.hpp"

namespace argo::awaiters {

/**
 * @brief Awaiter that implements parallel execution of a task within another
 * task scope
 */
template <typename T>
class async final {
 public:
  explicit constexpr async(task<T> && task) noexcept : task_(std::move(task)) {}

 public:
  constexpr auto operator co_await() && noexcept -> auto {
    class async_awaiter final {
     public:
      explicit constexpr async_awaiter(task<T> task) noexcept
          : task_(std::move(task)) {}

     public:
      constexpr auto await_ready() const noexcept -> bool { return true; }

      constexpr void await_suspend() const noexcept { std::unreachable(); }

      constexpr auto await_resume() -> task_future<T> {
        execution_context::this_executor().execute(task_.get_execution_token());
        return std::move(task_).get_future();
      }

     private:
      task<T> task_;
    };

    return async_awaiter(std::move(task_));
  }

 private:
  task<T> task_;
};

}  // namespace argo::awaiters
