#pragma once

#include <format>
#include <string>

#include "net/network_byte_order.hpp"

namespace core::net::inet {

/**
 * @brief IPv4 address
 */
class ip final {
 public:
  static const ip& kLoopback() noexcept;
  static const ip& kNonRoutable() noexcept;
  static const ip& kAny() noexcept;
  static const ip& kBroadcast() noexcept;

 public:
  explicit ip(std::uint32_t bytes) noexcept;
  ip(std::uint32_t bytes, network_byte_order_t) noexcept;
  explicit ip(std::string_view string);

 public:
  constexpr bool operator==(const ip&) const = default;
  constexpr bool operator!=(const ip&) const = default;

 public:
  std::uint32_t get_bytes() const noexcept;
  std::uint32_t get_bytes(network_byte_order_t) const noexcept;

 public:
  std::string to_string() const;

 private:
  std::uint32_t data_;
};

}  // namespace core::net::inet

template <>
struct std::formatter<core::net::inet::ip> {
  template <typename FormatContext>
  constexpr auto parse(FormatContext& ctx) const noexcept {
    return ctx.begin();
  }

  template <typename FormatContext>
  constexpr auto format(const core::net::inet::ip& ip,
                        FormatContext& ctx) const {
    return std::format_to(ctx.out(), "{}", ip.to_string());
  }
};
