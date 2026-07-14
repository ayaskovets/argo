#pragma once

#include <coroutine>

#include <swiss/mixins.hpp>

namespace argo::awaiters {

/**
 * @brief A special kind of awaiter for emulating return to the caller with
 * coroutines
 */
class continuation_awaiter final : public swiss::non_copyable,
                                   public swiss::non_movable {
 public:
  explicit constexpr continuation_awaiter(
      std::coroutine_handle<> continuation) noexcept
      : continuation_(continuation) {}

 public:
  constexpr auto await_ready() const noexcept -> bool {
    return !continuation_ || continuation_.done();
  }

  constexpr auto await_suspend([[maybe_unused]] std::coroutine_handle<> parent)
      const noexcept -> std::coroutine_handle<> {
    return continuation_ ? continuation_ : std::noop_coroutine();
  }

  constexpr void await_resume() const noexcept {}

 private:
  std::coroutine_handle<> continuation_;
};

}  // namespace argo::awaiters
