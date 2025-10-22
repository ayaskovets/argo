#pragma once

#include <cstdint>
#include <format>

namespace sandbox::net {

/**
 * @brief Address family
 */
enum class family : std::uint8_t {
  kUnspecified,
  kInet,
  kInet6,
  kUnix,
};

}  // namespace sandbox::net

template <>
class std::formatter<sandbox::net::family> {
 public:
  template <typename FormatContext>
  constexpr auto parse(FormatContext& ctx) const noexcept {
    return ctx.begin();
  }

  template <typename FormatContext>
  constexpr auto format(const sandbox::net::family& family,
                        FormatContext& ctx) const noexcept {
    switch (family) {
      case sandbox::net::family::kUnspecified:
        return std::format_to(ctx.out(), "AF_UNSPEC");
      case sandbox::net::family::kInet:
        return std::format_to(ctx.out(), "AF_INET");
      case sandbox::net::family::kInet6:
        return std::format_to(ctx.out(), "AF_INET6");
      case sandbox::net::family::kUnix:
        return std::format_to(ctx.out(), "AF_UNIX");
    }
  }
};
