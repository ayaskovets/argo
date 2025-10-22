#pragma once

#include <cstdint>
#include <format>

namespace sandbox::logging {

enum class level : std::uint8_t {
  kDebug,
  kInfo,
  kWarn,
  kError,
};

}  // namespace sandbox::logging

template <>
class std::formatter<sandbox::logging::level> {
 public:
  template <typename FormatContext>
  constexpr auto parse(FormatContext& ctx) const noexcept {
    return ctx.begin();
  }

  template <typename FormatContext>
  constexpr auto format(const sandbox::logging::level& level,
                        FormatContext& ctx) const noexcept {
    switch (level) {
      case sandbox::logging::level::kDebug:
        return std::format_to(ctx.out(), "DEBUG");
      case sandbox::logging::level::kInfo:
        return std::format_to(ctx.out(), "INFO");
      case sandbox::logging::level::kWarn:
        return std::format_to(ctx.out(), "WARN");
      case sandbox::logging::level::kError:
        return std::format_to(ctx.out(), "ERROR");
    }
  }
};
