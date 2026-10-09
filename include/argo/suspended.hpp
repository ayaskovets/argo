#pragma once

#include <coroutine>

namespace argo {

// TODO: rework this

/**
 * @brief Wrapped coroutine handle with a fixed set of possible operations.
 * Passed to await_suspend() instead of a plain std::coroutine_handle
 */
class suspended final {
 public:
  constexpr suspended() noexcept = default;
  explicit constexpr suspended(std::coroutine_handle<> handle) noexcept
      : handle_(handle) {}

 public:
  std::coroutine_handle<> handle_;
};

}  // namespace argo
