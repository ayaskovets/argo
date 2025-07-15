#pragma once

#include <cstddef>

namespace core::utils {

constexpr const std::size_t kCacheLineSize = 64;

// NOTE: a constant representing the same as std::dynamic_extent for a given
// integral type
template <std::unsigned_integral T>
constexpr const T kRuntimeDefinedCapacity = 0;

}  // namespace core::utils
