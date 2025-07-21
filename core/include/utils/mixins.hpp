#pragma once

namespace core::utils {

/**
 * @brief Mix-in to restrict copying
 *
 * Please follow the rule of five for all derived types
 */
struct non_copyable {
  constexpr non_copyable() noexcept = default;
  constexpr non_copyable(const non_copyable&) = delete;
  constexpr non_copyable& operator=(const non_copyable&) = delete;
};

/**
 * @brief Mix-in to restrict moving
 *
 * Please follow the rule of five for all derived types
 */
struct non_movable {
  constexpr non_movable() noexcept = default;
  constexpr non_movable(non_copyable&&) = delete;
  constexpr non_movable& operator=(non_movable&&) = delete;
};

}  // namespace core::utils
