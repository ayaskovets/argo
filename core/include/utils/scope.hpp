#pragma once

#include <concepts>
#include <utility>

#include "utils/mixins.hpp"

namespace core::utils {

/**
 * @brief Declarative way to do an noexcept operation when exiting a scope
 */
template <std::invocable T>
class scope_exit final : public utils::non_copyable, public utils::non_movable {
 private:
  static_assert(noexcept(std::declval<T>()()));

 public:
  constexpr explicit scope_exit(T&& finally)
      : finally_(std::forward<T>(finally)) {}
  constexpr ~scope_exit() noexcept { finally_(); }

 private:
  const T& finally_;
};

}  // namespace core::utils
