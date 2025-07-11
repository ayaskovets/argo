#pragma once

#include <cstdint>
#include <format>

namespace core::net::inet {

class port final {
 public:
  struct network_byte_order_t final {};

 public:
  explicit port(std::uint16_t port) noexcept;
  port(std::uint16_t port, network_byte_order_t) noexcept;

 public:
  constexpr bool operator==(const port&) const noexcept = default;
  constexpr bool operator!=(const port&) const noexcept = default;

 public:
  std::uint16_t get_bytes() const noexcept;
  std::uint16_t get_bytes(network_byte_order_t) const noexcept;

 public:
  std::string to_string() const noexcept;

 private:
  std::uint16_t port_;
};

}  // namespace core::net::inet

template <>
struct std::formatter<core::net::inet::port> {
  template <class FormatContext>
  constexpr auto parse(FormatContext& ctx) const noexcept {
    return ctx.begin();
  }

  template <class FormatContext>
  constexpr auto format(const core::net::inet::port& port,
                        FormatContext& ctx) const {
    return std::format_to(ctx.out(), "{}", port.to_string());
  }
};
