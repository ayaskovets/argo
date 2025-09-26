#pragma once

#include <cstdint>
#include <format>

namespace core::net::sockets {

/**
 * @brief Specific protocol
 */
enum class protocol : std::uint8_t {
  kUnspecified,
  kTcp,
  kUdp,
};

}  // namespace core::net::sockets

template <>
class std::formatter<core::net::sockets::protocol> {
 public:
  template <typename FormatContext>
  constexpr auto parse(FormatContext& ctx) const noexcept {
    return ctx.begin();
  }

  template <typename FormatContext>
  constexpr auto format(const core::net::sockets::protocol& protocol,
                        FormatContext& ctx) const {
    switch (protocol) {
      case core::net::sockets::protocol::kUnspecified:
        return std::format_to(ctx.out(), "UNSPECIFIED");
      case core::net::sockets::protocol::kTcp:
        return std::format_to(ctx.out(), "IPPROTO_TCP");
      case core::net::sockets::protocol::kUdp:
        return std::format_to(ctx.out(), "IPPROTO_UDP");
    }
  }
};
