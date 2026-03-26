#pragma once

#include <wip/net/base_sockaddr.hpp>
#include <wip/net/inet/ip.hpp>
#include <wip/net/inet/port.hpp>

namespace wip::net::inet {

/**
 * @brief IPv4 address:port pair
 */
class sockaddr final : public net::base_sockaddr {
 public:
  sockaddr(net::inet::ip ip, net::inet::port port) noexcept(false);

 public:
  net::inet::ip get_ip() const noexcept;
  net::inet::port get_port() const noexcept;
};

}  // namespace wip::net::inet

template <>
class std::formatter<wip::net::inet::sockaddr> {
 public:
  template <typename FormatContext>
  constexpr auto parse(FormatContext& ctx) const noexcept {
    return ctx.begin();
  }

  template <typename FormatContext>
  constexpr auto format(const wip::net::inet::sockaddr& sockaddr,
                        FormatContext& ctx) const noexcept(false) {
    return std::format_to(ctx.out(), "{}", sockaddr.to_string());
  }
};
