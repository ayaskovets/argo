#pragma once

#include "../execution_context.hpp"

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
  template <typename Caller>
  constexpr void await_suspend(Caller && caller) const {
    this_executor::execute(std::forward<Caller>(caller));
  }

  constexpr void await_resume() const noexcept {}
};

}  // namespace argo::awaiters

/**
 * @section Utility awaiter constructors
 */
namespace argo::awaiters {

constexpr auto yield() noexcept -> yield_awaiter { return {}; }

}  // namespace argo::awaiters
