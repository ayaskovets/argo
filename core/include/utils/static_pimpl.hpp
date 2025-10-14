#pragma once

#include <concepts>
#include <cstddef>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

namespace core::utils {

/**
 * @brief Pimpl without dynamic allocations
 *
 * Uses pointer-like syntax for access to the implementation
 *
 * Size and alignment must be manually set for the specific instance
 */
template <typename T, std::size_t Size, std::size_t Alignment>
class static_pimpl final {
 public:
  template <typename... Args>
  constexpr explicit static_pimpl(Args&&... args) noexcept(
      std::is_nothrow_constructible_v<T, Args...>) {
    std::construct_at(operator->(), std::forward<Args>(args)...);
  }

  constexpr static_pimpl(const static_pimpl& that) noexcept(
      std::is_nothrow_copy_constructible_v<T>) {
    std::construct_at(operator->(), that.operator*());
  }

  constexpr static_pimpl& operator=(const static_pimpl& that) noexcept(
      std::is_nothrow_copy_assignable_v<T>) {
    operator*() = that.operator*();
    return *this;
  }

  constexpr static_pimpl(static_pimpl&& that) noexcept(
      std::is_nothrow_move_constructible_v<T>) {
    std::construct_at(operator->(), std::move(that.operator*()));
  }

  constexpr static_pimpl& operator=(static_pimpl&& that) noexcept(
      std::is_nothrow_move_assignable_v<T>) {
    operator*() = std::move(that.operator*());
    return *this;
  }

  constexpr ~static_pimpl() noexcept {
    static_assert(sizeof(T) == Size);
    static_assert(alignof(T) == Alignment);
    std::destroy_at(operator->());
  }

 public:
  template <typename... Args>
  constexpr void emplace(Args&&... args) noexcept(
      std::is_nothrow_constructible_v<T, Args...>)
    requires std::constructible_from<T, Args...> &&
             std::is_nothrow_destructible_v<T>
  {
    std::destroy_at(operator->());
    std::construct_at(operator->(), std::forward<Args>(args)...);
  }

 public:
  constexpr T& operator*() noexcept {
    return *std::launder(reinterpret_cast<T*>(storage_));
  }
  constexpr const T& operator*() const noexcept {
    return *std::launder(reinterpret_cast<const T*>(storage_));
  }

  constexpr T* operator->() noexcept {
    return std::launder(reinterpret_cast<T*>(storage_));
  }
  constexpr const T* operator->() const noexcept {
    return std::launder(reinterpret_cast<const T*>(storage_));
  }

 private:
  alignas(Alignment) std::byte storage_[Size];
};

}  // namespace core::utils
