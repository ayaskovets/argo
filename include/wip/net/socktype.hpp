#pragma once

#include <cstdint>
#include <format>

namespace wip::net {

/**
 * @brief Protocol type
 */
enum class socktype : std::uint8_t {
  kStream,
  kDgram,
  kRaw,
  kRdm,
  kSeqpacket,
};

}  // namespace wip::net

template <>
class std::formatter<wip::net::socktype> {
 public:
  template <typename FormatContext>
  constexpr auto parse(FormatContext& ctx) const noexcept {
    return ctx.begin();
  }

  template <typename FormatContext>
  constexpr auto format(const wip::net::socktype& type,
                        FormatContext& ctx) const noexcept {
    switch (type) {
      case wip::net::socktype::kStream:
        return std::format_to(ctx.out(), "SOCK_STREAM");
      case wip::net::socktype::kDgram:
        return std::format_to(ctx.out(), "SOCK_DGRAM");
      case wip::net::socktype::kRaw:
        return std::format_to(ctx.out(), "SOCK_RAW");
      case wip::net::socktype::kRdm:
        return std::format_to(ctx.out(), "SOCK_RDM");
      case wip::net::socktype::kSeqpacket:
        return std::format_to(ctx.out(), "SOCK_SEQPACKET");
    }
  }
};
