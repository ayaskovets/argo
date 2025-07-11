#pragma once

#include <cstdint>
#include <format>

namespace core::net::sockets {

enum class family : std::uint8_t {
  kUnspecified,
  kInet,
  kInet6,
  kUnix,
};

}  // namespace core::net::sockets

template <>
struct std::formatter<core::net::sockets::family> {
  template <class FormatContext>
  constexpr auto parse(FormatContext& ctx) const noexcept {
    return ctx.begin();
  }

  template <class FormatContext>
  constexpr auto format(const core::net::sockets::family& family,
                        FormatContext& ctx) const {
    switch (family) {
      case core::net::sockets::family::kUnspecified:
        return std::format_to(ctx.out(), "AF_UNSPEC");
      case core::net::sockets::family::kInet:
        return std::format_to(ctx.out(), "AF_INET");
      case core::net::sockets::family::kInet6:
        return std::format_to(ctx.out(), "AF_INET6");
      case core::net::sockets::family::kUnix:
        return std::format_to(ctx.out(), "AF_UNIX");
    }
  }
};
