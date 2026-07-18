#pragma once

#include <swiss/mixins.hpp>

#include "../const_coroutine.hpp"

namespace argo::awaiters {

/**
 * @brief A special kind of awaiter that implements symmetric transfer
 */
class continuation_awaiter final : public swiss::non_copyable,
                                   public swiss::non_movable {
 public:
  explicit constexpr continuation_awaiter(
      const_coroutine_handle<> continuation) noexcept
      : continuation_(continuation) {}

 public:
  constexpr auto await_ready() const noexcept -> bool { return false; }

  constexpr auto await_suspend(std::coroutine_handle<>) const noexcept
      -> std::coroutine_handle<> {
    return continuation_ ? continuation_.to_mutable_handle()
                         : std::noop_coroutine();
  }

  constexpr void await_resume() const noexcept {}

 private:
  const_coroutine_handle<> continuation_;
};

}  // namespace argo::awaiters

/**
 * @section Utility awaiter constructors
 */
namespace argo::awaiters {

constexpr auto continuation(const_coroutine_handle<> continuation) noexcept
    -> continuation_awaiter {
  return continuation_awaiter(continuation);
}

}  // namespace argo::awaiters
