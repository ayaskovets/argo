#pragma once

#include <cstdint>
#include <format>

namespace wip::net {

/**
 * @brief Address family
 */
enum class family : std::uint8_t {
  kUnspecified,
  kInet,
  kInet6,
  kUnix,
};

}  // namespace wip::net

template <>
class std::formatter<wip::net::family> {
 public:
  template <typename FormatContext>
  constexpr auto parse(FormatContext& ctx) const noexcept {
    return ctx.begin();
  }

  template <typename FormatContext>
  constexpr auto format(const wip::net::family& family,
                        FormatContext& ctx) const noexcept {
    switch (family) {
      case wip::net::family::kUnspecified:
        return std::format_to(ctx.out(), "AF_UNSPEC");
      case wip::net::family::kInet:
        return std::format_to(ctx.out(), "AF_INET");
      case wip::net::family::kInet6:
        return std::format_to(ctx.out(), "AF_INET6");
      case wip::net::family::kUnix:
        return std::format_to(ctx.out(), "AF_UNIX");
    }
  }
};
