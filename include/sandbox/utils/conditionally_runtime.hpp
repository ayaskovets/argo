#pragma once

#include <type_traits>
#include <utility>

namespace sandbox::utils {

/**
 * @brief Conditionally runtime-stored value
 *
 * Uses pointer-like syntax for access to the stored value
 *
 * Add [[no_unique_address]] to a compile-time version of an object of this
 * class to optimize away storage at runtime
 */
template <typename T, bool IsRuntime, T CompileTimeValue = T()>
class conditionally_runtime;

template <typename T, T CompileTimeValue>
class conditionally_runtime<T, true, CompileTimeValue> final {
 public:
  using value_type = T;
  using reference = T&;
  using const_reference = T&;
  using pointer = T*;
  using const_pointer = const T*;

 public:
  constexpr conditionally_runtime() = delete;
  constexpr conditionally_runtime(T value) noexcept(
      std::is_nothrow_move_constructible_v<T>)
      : value_(std::move(value)) {}

 public:
  constexpr T& operator*() noexcept { return value_; }
  constexpr const T& operator*() const noexcept { return value_; }
  constexpr T* operator->() noexcept { return &value_; };
  constexpr const T* operator->() const noexcept { return &value_; };

 private:
  T value_;
};

template <typename T, T CompileTimeValue>
class conditionally_runtime<T, false, CompileTimeValue> final {
 public:
  constexpr conditionally_runtime() noexcept = default;

 public:
  constexpr const T& operator*() const noexcept { return value_; }
  constexpr const T* operator->() const noexcept { return &value_; };

 private:
  static inline const constinit T value_ = CompileTimeValue;
};

}  // namespace sandbox::utils
