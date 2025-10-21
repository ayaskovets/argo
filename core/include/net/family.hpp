#pragma once

#include <cstdint>
#include <format>

namespace core::net {

/**
 * @brief Address family
 */
enum class family : std::uint8_t {
  kUnspecified,
  kInet,
  kInet6,
  kUnix,
};

}  // namespace core::net

template <>
class std::formatter<core::net::family> {
 public:
  template <typename FormatContext>
  constexpr auto parse(FormatContext& ctx) const noexcept {
    return ctx.begin();
  }

  template <typename FormatContext>
  constexpr auto format(const core::net::family& family,
                        FormatContext& ctx) const noexcept {
    switch (family) {
      case core::net::family::kUnspecified:
        return std::format_to(ctx.out(), "AF_UNSPEC");
      case core::net::family::kInet:
        return std::format_to(ctx.out(), "AF_INET");
      case core::net::family::kInet6:
        return std::format_to(ctx.out(), "AF_INET6");
      case core::net::family::kUnix:
        return std::format_to(ctx.out(), "AF_UNIX");
    }
  }
};
