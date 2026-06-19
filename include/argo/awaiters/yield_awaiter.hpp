#pragma once

#include "../this_executor.hpp"

namespace argo::awaiters {

/**
 * @brief Yield execution to another task
 */
class yield_awaiter final {
 public:
  [[nodiscard]] constexpr auto await_ready() const noexcept -> bool {
    return false;
  }

  constexpr void await_suspend(
      std::coroutine_handle<> const parent) const noexcept {
    this_executor::yield(parent);
  }

  constexpr void await_resume() const noexcept {}
};

}  // namespace argo::awaiters
