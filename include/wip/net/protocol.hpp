#pragma once

#include <cstdint>
#include <format>

namespace wip::net {

/**
 * @brief Specific protocol
 */
enum class protocol : std::uint8_t {
  kUnspecified,
  kTcp,
  kUdp,
};

}  // namespace wip::net

template <>
class std::formatter<wip::net::protocol> {
 public:
  template <typename FormatContext>
  constexpr auto parse(FormatContext& ctx) const noexcept {
    return ctx.begin();
  }

  template <typename FormatContext>
  constexpr auto format(const wip::net::protocol& protocol,
                        FormatContext& ctx) const noexcept {
    switch (protocol) {
      case wip::net::protocol::kUnspecified:
        return std::format_to(ctx.out(), "UNSPECIFIED");
      case wip::net::protocol::kTcp:
        return std::format_to(ctx.out(), "IPPROTO_TCP");
      case wip::net::protocol::kUdp:
        return std::format_to(ctx.out(), "IPPROTO_UDP");
    }
  }
};
