#pragma once

#include <coroutine>

namespace argo::awaiters {

/**
 * @brief Utility awaiter that implements symmetric transfer
 */
class continuation_awaiter final {
 public:
  explicit constexpr continuation_awaiter(
      std::coroutine_handle<> continuation) noexcept
      : continuation_(continuation) {}

 public:
  constexpr auto await_ready() const noexcept -> bool { return false; }

  constexpr auto await_suspend(auto &&) const noexcept
      -> std::coroutine_handle<> {
    return continuation_ ? continuation_ : std::noop_coroutine();
  }

  constexpr void await_resume() const noexcept {}

 private:
  std::coroutine_handle<> continuation_;
};

}  // namespace argo::awaiters

/**
 * @section Utility awaiter constructors
 */
namespace argo::awaiters {

constexpr auto continuation(std::coroutine_handle<> continuation) noexcept
    -> continuation_awaiter {
  return continuation_awaiter(continuation);
}

}  // namespace argo::awaiters
