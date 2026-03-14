#pragma once

#include <cstdint>
#include <format>

namespace wip::logging {

enum class level : std::uint8_t {
  kDebug,
  kInfo,
  kWarn,
  kError,
};

}  // namespace wip::logging

template <>
class std::formatter<wip::logging::level> {
 public:
  template <typename FormatContext>
  constexpr auto parse(FormatContext& ctx) const noexcept {
    return ctx.begin();
  }

  template <typename FormatContext>
  constexpr auto format(const wip::logging::level& level,
                        FormatContext& ctx) const noexcept {
    switch (level) {
      case wip::logging::level::kDebug:
        return std::format_to(ctx.out(), "DEBUG");
      case wip::logging::level::kInfo:
        return std::format_to(ctx.out(), "INFO");
      case wip::logging::level::kWarn:
        return std::format_to(ctx.out(), "WARN");
      case wip::logging::level::kError:
        return std::format_to(ctx.out(), "ERROR");
    }
  }
};
