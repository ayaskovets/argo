#pragma once

#include <cstdint>
#include <format>

namespace core::net {

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

}  // namespace core::net

template <>
class std::formatter<core::net::socktype> {
 public:
  template <typename FormatContext>
  constexpr auto parse(FormatContext& ctx) const noexcept {
    return ctx.begin();
  }

  template <typename FormatContext>
  constexpr auto format(const core::net::socktype& type,
                        FormatContext& ctx) const noexcept {
    switch (type) {
      case core::net::socktype::kStream:
        return std::format_to(ctx.out(), "SOCK_STREAM");
      case core::net::socktype::kDgram:
        return std::format_to(ctx.out(), "SOCK_DGRAM");
      case core::net::socktype::kRaw:
        return std::format_to(ctx.out(), "SOCK_RAW");
      case core::net::socktype::kRdm:
        return std::format_to(ctx.out(), "SOCK_RDM");
      case core::net::socktype::kSeqpacket:
        return std::format_to(ctx.out(), "SOCK_SEQPACKET");
    }
  }
};
