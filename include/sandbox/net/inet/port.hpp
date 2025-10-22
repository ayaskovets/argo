#pragma once

#include <cstdint>
#include <format>
#include <sandbox/net/network_byte_order.hpp>

namespace sandbox::net::inet {

/**
 * @brief IP port
 */
class port final {
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

}  // namespace sandbox::net::inet

template <>
class std::formatter<sandbox::net::inet::port> {
 public:
  template <typename FormatContext>
  constexpr auto parse(FormatContext& ctx) const noexcept {
    return ctx.begin();
  }

  template <typename FormatContext>
  constexpr auto format(const sandbox::net::inet::port& port,
                        FormatContext& ctx) const noexcept {
    return std::format_to(ctx.out(), "{}", port.to_string());
  }
};
