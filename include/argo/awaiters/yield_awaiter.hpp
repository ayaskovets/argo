#pragma once

#include "../execution_context.hpp"

namespace argo::awaiters {

/**
 * @brief Utility awaiter that puts task back onto the executor input queue
 * bypassing the output channel path through a task scheduler
 */
class yield final {
 public:
  constexpr auto operator co_await() && noexcept -> auto {
    class yield_awaiter final {
     public:
      constexpr auto await_ready() const noexcept -> bool { return false; }

      constexpr void await_suspend(execution_token token) const {
        execution_context::this_executor().execute(std::move(token));
      }

      constexpr void await_resume() const noexcept {}
    };

    return yield_awaiter();
  }
};

}  // namespace argo::awaiters
