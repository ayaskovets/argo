#pragma once

#include <concepts>

namespace sandbox::utils {

template <std::integral T>
constexpr bool is_power_of_two(T value) noexcept {
  return value > 0 && ((value & (value - static_cast<T>(1))) == 0);
}

template <std::integral T>
constexpr bool is_even(T value) noexcept {
  return !(value & 1);
}

}  // namespace sandbox::utils
