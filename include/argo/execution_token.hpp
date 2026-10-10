#pragma once

#include <coroutine>
#include <utility>

#include <swiss/mixins.hpp>

namespace argo {

// TODO: rework this

/**
 * @brief Wrapped coroutine handle with a fixed set of possible operations.
 * Passed to await_suspend() instead of a plain std::coroutine_handle
 */
// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions)
class execution_token final : public swiss::non_copyable {
 public:
  constexpr execution_token() noexcept = default;
  explicit constexpr execution_token(std::coroutine_handle<> handle) noexcept
      : handle_(handle) {}

  constexpr execution_token(execution_token && that) noexcept
      : handle_(std::exchange(that.handle_, nullptr)) {}

  constexpr auto operator=(execution_token && that) noexcept
      -> execution_token & {
    std::swap(handle_, that.handle_);
    return *this;
  }

 public:
  constexpr void resume() const noexcept { handle_.resume(); }

 public:
  std::coroutine_handle<> handle_;
};

}  // namespace argo
