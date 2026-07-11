#pragma once

#include <swiss/mixins.hpp>

#include "../this_executor.hpp"

namespace argo::awaiters {

/**
 * @brief A special kind of awaiter that puts task back onto the executor input
 * queue bypassing any scheduler
 */
class yield_awaiter final : public swiss::non_copyable,
                            public swiss::non_movable {
 public:
  constexpr auto await_ready() const noexcept -> bool { return false; }

  /**
   * @note Noexcept if this_executor::yield is noexcept
   */
  constexpr void await_suspend(std::coroutine_handle<> const parent) const {
    this_executor::yield(parent);
  }

  constexpr void await_resume() const noexcept {}
};

}  // namespace argo::awaiters
