#pragma once

#include <format>
#include <sandbox/net/network_byte_order.hpp>
#include <string>

namespace sandbox::net::inet {

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
  explicit ip(std::string_view string) noexcept(false);

 public:
  constexpr bool operator==(const ip&) const noexcept = default;
  constexpr bool operator!=(const ip&) const noexcept = default;

 public:
  std::uint32_t get_bytes() const noexcept;
  std::uint32_t get_bytes(network_byte_order_t) const noexcept;

 public:
  std::string to_string() const noexcept(false);

 private:
  std::uint32_t data_;
};

}  // namespace sandbox::net::inet

template <>
class std::formatter<sandbox::net::inet::ip> {
 public:
  template <typename FormatContext>
  constexpr auto parse(FormatContext& ctx) const noexcept {
    return ctx.begin();
  }

  template <typename FormatContext>
  constexpr auto format(const sandbox::net::inet::ip& ip,
                        FormatContext& ctx) const noexcept(false) {
    return std::format_to(ctx.out(), "{}", ip.to_string());
  }
};
