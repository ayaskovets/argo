#pragma once

#include "../execution_context.hpp"

namespace argo::awaiters {

/**
 * @brief Utility awaiter that implements symmetric transfer
 */
class switch_to_awaiter final {
 public:
  explicit constexpr switch_to_awaiter(execution_context & ctx) noexcept
      : ctx_(ctx) {}

 public:
  constexpr auto await_ready() const noexcept -> bool { return true; }
  constexpr void await_suspend(auto &&) const noexcept { std::unreachable(); }
  constexpr void await_resume() const noexcept {
    execution_context::this_executor_ = &ctx_;
  }

 private:
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-const-or-ref-data-members)
  execution_context & ctx_;
};

}  // namespace argo::awaiters

/**
 * @section Awaiter constructors
 */
namespace argo::awaiters {

constexpr auto switch_to(execution_context & ctx) noexcept
    -> switch_to_awaiter {
  return switch_to_awaiter(ctx);
}

}  // namespace argo::awaiters
