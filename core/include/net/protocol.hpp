#pragma once

#include <cstdint>
#include <format>

namespace core::net {

/**
 * @brief Specific protocol
 */
enum class protocol : std::uint8_t {
  kUnspecified,
  kTcp,
  kUdp,
};

}  // namespace core::net

template <>
class std::formatter<core::net::protocol> {
 public:
  template <typename FormatContext>
  constexpr auto parse(FormatContext& ctx) const noexcept {
    return ctx.begin();
  }

  template <typename FormatContext>
  constexpr auto format(const core::net::protocol& protocol,
                        FormatContext& ctx) const noexcept {
    switch (protocol) {
      case core::net::protocol::kUnspecified:
        return std::format_to(ctx.out(), "UNSPECIFIED");
      case core::net::protocol::kTcp:
        return std::format_to(ctx.out(), "IPPROTO_TCP");
      case core::net::protocol::kUdp:
        return std::format_to(ctx.out(), "IPPROTO_UDP");
    }
  }
};
