#pragma once

#include <format>
#include <span>
#include <string>

namespace core::net::inet6 {

class ip final {
 public:
  static const ip& kLoopback() noexcept;
  static const ip& kNonRoutable() noexcept;
  static const ip& kAny() noexcept;

 public:
  struct network_byte_order_t final {};

 public:
  ip(std::span<const std::byte, 16> bytes, network_byte_order_t) noexcept;
  explicit ip(std::string_view string);

 public:
  constexpr bool operator==(const ip&) const = default;
  constexpr bool operator!=(const ip&) const = default;

 public:
  const std::array<std::byte, 16>& get_bytes(
      network_byte_order_t) const noexcept;

 public:
  std::string to_string() const;

 private:
  std::array<std::byte, 16> data_;
};

}  // namespace core::net::inet6

template <>
struct std::formatter<core::net::inet6::ip> {
  template <class FormatContext>
  constexpr auto parse(FormatContext& ctx) const noexcept {
    return ctx.begin();
  }

  template <class FormatContext>
  constexpr auto format(const core::net::inet6::ip& ip,
                        FormatContext& ctx) const {
    return std::format_to(ctx.out(), "{}", ip.to_string());
  }
};
