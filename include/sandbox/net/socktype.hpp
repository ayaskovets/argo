#pragma once

#include <cstdint>
#include <format>

namespace sandbox::net {

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

}  // namespace sandbox::net

template <>
class std::formatter<sandbox::net::socktype> {
 public:
  template <typename FormatContext>
  constexpr auto parse(FormatContext& ctx) const noexcept {
    return ctx.begin();
  }

  template <typename FormatContext>
  constexpr auto format(const sandbox::net::socktype& type,
                        FormatContext& ctx) const noexcept {
    switch (type) {
      case sandbox::net::socktype::kStream:
        return std::format_to(ctx.out(), "SOCK_STREAM");
      case sandbox::net::socktype::kDgram:
        return std::format_to(ctx.out(), "SOCK_DGRAM");
      case sandbox::net::socktype::kRaw:
        return std::format_to(ctx.out(), "SOCK_RAW");
      case sandbox::net::socktype::kRdm:
        return std::format_to(ctx.out(), "SOCK_RDM");
      case sandbox::net::socktype::kSeqpacket:
        return std::format_to(ctx.out(), "SOCK_SEQPACKET");
    }
  }
};
