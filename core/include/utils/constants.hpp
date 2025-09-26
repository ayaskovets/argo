#pragma once

#include <cstddef>

namespace core::utils {

/**
 * @brief Constant semantically similar to std::dynamic_extent but with value 0
 */
template <std::unsigned_integral T>
constexpr const T kRuntimeDefinedCapacity = 0;

}  // namespace core::utils
