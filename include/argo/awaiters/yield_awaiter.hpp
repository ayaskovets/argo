#pragma once

#include "../this_executor.hpp"

namespace argo::awaiters {

/**
 * @brief A special kind of awaiter that puts task back onto the executor input
 * queue bypassing any scheduler
 */
class yield_awaiter final {
 public:
  constexpr auto await_ready() const noexcept -> bool { return false; }

  constexpr void await_suspend(
      std::coroutine_handle<> const parent) const noexcept {
    this_executor::yield(parent);
  }

  constexpr void await_resume() const noexcept {}
};

}  // namespace argo::awaiters
