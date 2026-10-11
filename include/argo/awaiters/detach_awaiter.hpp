#pragma once

#include "../execution_context.hpp"
#include "../task.hpp"

namespace argo::awaiters {

/**
 * @brief Awaiter that implements explicit background execution of a task.
 * Promise frame is cleaned up automatically
 */
class detach final {
 public:
  explicit constexpr detach(task<void> task) noexcept
      : task_(std::move(task)) {}

 public:
  constexpr auto operator co_await() && noexcept -> auto {
    class detach_awaiter final {
     public:
      explicit constexpr detach_awaiter(task<void> task) noexcept
          : task_(std::move(task)) {}

     public:
      constexpr auto await_ready() const noexcept -> bool { return true; }

      constexpr void await_suspend() const noexcept { std::unreachable(); }

      constexpr void await_resume() {
        execution_context::this_executor().execute(task_.get_execution_token());
      }

     private:
      task<void> task_;
    };

    return detach_awaiter(std::move(task_));
  }

 private:
  task<void> task_;
};

}  // namespace argo::awaiters
