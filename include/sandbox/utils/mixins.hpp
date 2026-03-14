#pragma once

namespace wip::utils {

/**
 * @brief Mix-in to restrict copying
 *
 * Please follow the rule of five for all derived types
 */
class non_copyable {
 public:
  constexpr non_copyable() noexcept = default;
  constexpr non_copyable(const non_copyable&) = delete;
  constexpr non_copyable& operator=(const non_copyable&) = delete;

 public:
  constexpr non_copyable(non_copyable&&) noexcept = default;
  constexpr non_copyable& operator=(non_copyable&&) noexcept = default;
};

/**
 * @brief Mix-in to restrict moving
 *
 * Please follow the rule of five for all derived types
 *
 * @warning Keep in mind that all operations with a type that prohibits only its
 * moves will fallback to copy
 */
class non_movable {
 public:
  constexpr non_movable() noexcept = default;
  constexpr non_movable(non_movable&&) = delete;
  constexpr non_movable& operator=(non_movable&&) = delete;

 public:
  constexpr non_movable(const non_movable&) noexcept = default;
  constexpr non_movable& operator=(const non_movable&) noexcept = default;
};

}  // namespace wip::utils
