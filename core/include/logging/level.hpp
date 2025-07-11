#pragma once

#include <cstdint>
#include <format>

namespace core::logging {

enum class level : std::uint8_t {
  kDebug,
  kInfo,
  kWarn,
  kError,
};

}  // namespace core::logging

template <>
struct std::formatter<core::logging::level> {
  template <class FormatContext>
  constexpr auto parse(FormatContext& ctx) const noexcept {
    return ctx.begin();
  }

  template <class FormatContext>
  constexpr auto format(const core::logging::level& level,
                        FormatContext& ctx) const {
    switch (level) {
      case core::logging::level::kDebug:
        return std::format_to(ctx.out(), "DEBUG");
      case core::logging::level::kInfo:
        return std::format_to(ctx.out(), "INFO");
      case core::logging::level::kWarn:
        return std::format_to(ctx.out(), "WARN");
      case core::logging::level::kError:
        return std::format_to(ctx.out(), "ERROR");
    }
  }
};
