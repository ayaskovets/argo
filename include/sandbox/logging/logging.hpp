#pragma once

#include <wip/logging/level.hpp>
#include <wip/logging/logger.hpp>

namespace wip::logging {

template <typename... Args>
constexpr decltype(auto) debug(std::format_string<Args...> fmt,
                               Args&&... args) noexcept(false) {
  return logging::logger::kDefault()(logging::level::kDebug, fmt,
                                     std::forward<Args>(args)...);
}

template <typename... Args>
constexpr decltype(auto) info(std::format_string<Args...> fmt,
                              Args&&... args) noexcept(false) {
  return logging::logger::kDefault()(logging::level::kInfo, fmt,
                                     std::forward<Args>(args)...);
}

template <typename... Args>
constexpr decltype(auto) warning(std::format_string<Args...> fmt,
                                 Args&&... args) noexcept(false) {
  return logging::logger::kDefault()(logging::level::kWarn, fmt,
                                     std::forward<Args>(args)...);
}

template <typename... Args>
constexpr decltype(auto) error(std::format_string<Args...> fmt,
                               Args&&... args) noexcept(false) {
  return logging::logger::kDefault()(logging::level::kError, fmt,
                                     std::forward<Args>(args)...);
}

}  // namespace wip::logging
