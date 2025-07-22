#pragma once

#include <format>
#include <string_view>

#include "logging/level.hpp"

namespace core::logging {

/**
 * @brief Log message decorator
 *
 */
class renderer final {
 public:
  static const renderer& kNoop();
  static const renderer& kColoredLeveled();
  static const renderer& kColoredLeveledTimestamped();
  static const renderer& kDefault();

 private:
  using render_t = std::string (*)(logging::level level, std::string_view fmt,
                                   std::format_args args);

 public:
  constexpr explicit renderer(render_t render) noexcept
      : render_(std::move(render)) {}

 public:
  constexpr std::string operator()(logging::level level, std::string_view fmt,
                                   std::format_args args) const {
    return render_(level, fmt, std::move(args));
  }

 private:
  render_t render_;
};

}  // namespace core::logging
