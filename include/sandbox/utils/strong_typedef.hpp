#pragma once

#include <cstddef>
#include <type_traits>

namespace sandbox::utils {

/**
 * @brief Pimpl without dynamic allocations
 *
 * Uses pointer-like syntax for access to the implementation
 *
 * Size and alignment must be manually set for the specific instance
 */
template <typename T>
class strong_typedef {
 public:
  using value_type = T;
  using reference = T&;
  using const_reference = T&;
  using pointer = T*;
  using const_pointer = const T*;

 public:
  template <typename... Args>
  constexpr explicit strong_typedef(Args&&... args) noexcept(
      std::is_nothrow_constructible_v<T, Args...>)
      : underlying_(std::forward<Args>(args)...) {}

 public:
  constexpr auto operator<=>(const strong_typedef& that) const noexcept =
      default;

 public:
  constexpr T& operator*() noexcept { return underlying_; }
  constexpr const T& operator*() const noexcept { return underlying_; }

  constexpr T* operator->() noexcept { return &underlying_; }
  constexpr const T* operator->() const noexcept { return &underlying_; }

 private:
  T underlying_;
};

}  // namespace sandbox::utils
