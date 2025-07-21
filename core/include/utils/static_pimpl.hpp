#pragma once

#include <cstddef>
#include <memory>

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
  constexpr explicit static_pimpl(Args&&... args) {
    std::construct_at(operator->(), std::forward<Args>(args)...);
  }

  constexpr static_pimpl(const static_pimpl& that) {
    std::construct_at(operator->(), that.operator*());
  }

  constexpr static_pimpl& operator=(const static_pimpl& that) {
    operator*() = that.operator*();
    return *this;
  }

  constexpr static_pimpl(static_pimpl&& that) {
    std::construct_at(operator->(), std::move(that.operator*()));
  }

  constexpr static_pimpl& operator=(static_pimpl&& that) {
    operator*() = std::move(that.operator*());
    return *this;
  }

  constexpr ~static_pimpl() noexcept {
    static_assert(sizeof(T) == Size);
    static_assert(alignof(T) == Alignment);
    std::destroy_at(operator->());
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
