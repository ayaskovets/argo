#pragma once

#include <cstddef>

namespace core::utils {

/**
 * @brief Poor man's std::hardware_destructive_interference_size
 */
constexpr const std::size_t kCacheLineSize = 64;

/**
 * @brief Constant semantically similar to std::dynamic_extent but with value 0
 */
template <std::unsigned_integral T>
constexpr const T kRuntimeDefinedCapacity = 0;

}  // namespace core::utils
