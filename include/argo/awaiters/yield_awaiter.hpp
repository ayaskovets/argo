#pragma once

#include "../execution_context.hpp"
#include "../task_ref.hpp"

namespace argo::awaiters {

/**
 * @brief A special kind of awaiter that puts task back onto the executor input
 * queue bypassing any scheduler
 */
class yield_awaiter final {
 public:
  constexpr auto await_ready() const noexcept -> bool { return false; }

  /**
   * @note Noexcept if this_executor::execute is noexcept
   */
  constexpr void await_suspend(task_ref caller) const {
    this_executor::execute(caller);
  }

  constexpr void await_resume() const noexcept {}
};

}  // namespace argo::awaiters

/**
 * @section Awaiter constructors
 */
namespace argo::awaiters {

constexpr auto yield() noexcept -> yield_awaiter { return {}; }

}  // namespace argo::awaiters
