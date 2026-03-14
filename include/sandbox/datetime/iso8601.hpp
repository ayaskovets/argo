#pragma once

#include <chrono>
#include <string>

namespace wip::datetime {

/**
 * @brief Serialize timestamp into ISO 8601 format
 *
 * @example YYYY-MM-DDThh:mm:ss.usZ
 */
std::string to_iso8601(
    std::chrono::system_clock::time_point time_point) noexcept;

}  // namespace wip::datetime
