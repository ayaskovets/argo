#pragma once

namespace core::utils {

/**
 * @brief Utility function to access pointers and references uniformly
 *
 * @warning Does not perform null pointer checks
 */
template <typename T>
constexpr auto& as_ref(T* value) noexcept {
  return *value;
}

template <typename T>
constexpr const auto& as_ref(const T* value) noexcept {
  return *value;
}

template <typename T>
constexpr auto& as_ref(T& value) noexcept {
  return value;
}

template <typename T>
constexpr const auto& as_ref(const T& value) noexcept {
  return value;
}

}  // namespace core::utils
