#pragma once

#include <cstdint>
#include <format>

namespace sandbox::net {

/**
 * @brief Specific protocol
 */
enum class protocol : std::uint8_t {
  kUnspecified,
  kTcp,
  kUdp,
};

}  // namespace sandbox::net

template <>
class std::formatter<sandbox::net::protocol> {
 public:
  template <typename FormatContext>
  constexpr auto parse(FormatContext& ctx) const noexcept {
    return ctx.begin();
  }

  template <typename FormatContext>
  constexpr auto format(const sandbox::net::protocol& protocol,
                        FormatContext& ctx) const noexcept {
    switch (protocol) {
      case sandbox::net::protocol::kUnspecified:
        return std::format_to(ctx.out(), "UNSPECIFIED");
      case sandbox::net::protocol::kTcp:
        return std::format_to(ctx.out(), "IPPROTO_TCP");
      case sandbox::net::protocol::kUdp:
        return std::format_to(ctx.out(), "IPPROTO_UDP");
    }
  }
};
