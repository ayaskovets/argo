#pragma once

#include <concepts>
#include <cstddef>

namespace sandbox::utils {

/**
 * @brief Type-parametrized std::dynamic_extent
 */
template <std::unsigned_integral T>
constexpr const T kDynamicExtent = 0;

}  // namespace sandbox::utils
