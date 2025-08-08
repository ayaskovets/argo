#pragma once

#include <cstdint>
#include <format>

namespace core::net::sockets {

/**
 * @brief Protocol type
 */
enum class type : std::uint8_t {
  kStream,
  kDgram,
  kRaw,
  kRdm,
  kSeqpacket,
};

}  // namespace core::net::sockets

template <>
struct std::formatter<core::net::sockets::type> {
  template <typename FormatContext>
  constexpr auto parse(FormatContext& ctx) const noexcept {
    return ctx.begin();
  }

  template <typename FormatContext>
  constexpr auto format(const core::net::sockets::type& type,
                        FormatContext& ctx) const {
    switch (type) {
      case core::net::sockets::type::kStream:
        return std::format_to(ctx.out(), "SOCK_STREAM");
      case core::net::sockets::type::kDgram:
        return std::format_to(ctx.out(), "SOCK_DGRAM");
      case core::net::sockets::type::kRaw:
        return std::format_to(ctx.out(), "SOCK_RAW");
      case core::net::sockets::type::kRdm:
        return std::format_to(ctx.out(), "SOCK_RDM");
      case core::net::sockets::type::kSeqpacket:
        return std::format_to(ctx.out(), "SOCK_SEQPACKET");
    }
  }
};
