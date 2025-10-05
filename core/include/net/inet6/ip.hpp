#pragma once

#include <format>
#include <span>
#include <string>

#include "net/network_byte_order.hpp"

namespace core::net::inet6 {

/**
 * @brief IPv6 address
 */
class ip final {
 public:
  static const ip& kLoopback() noexcept;
  static const ip& kNonRoutable() noexcept;
  static const ip& kAny() noexcept;

 public:
  ip(std::span<const std::byte, 16> bytes, network_byte_order_t) noexcept;
  explicit ip(std::string_view string) noexcept(false);

 public:
  constexpr bool operator==(const ip&) const noexcept = default;
  constexpr bool operator!=(const ip&) const noexcept = default;

 public:
  const std::array<std::byte, 16>& get_bytes(
      network_byte_order_t) const noexcept;

 public:
  std::string to_string() const noexcept(false);

 private:
  std::array<std::byte, 16> data_;
};

}  // namespace core::net::inet6

template <>
class std::formatter<core::net::inet6::ip> {
 public:
  template <typename FormatContext>
  constexpr auto parse(FormatContext& ctx) const noexcept {
    return ctx.begin();
  }

  template <typename FormatContext>
  constexpr auto format(const core::net::inet6::ip& ip,
                        FormatContext& ctx) const noexcept(false) {
    return std::format_to(ctx.out(), "{}", ip.to_string());
  }
};
