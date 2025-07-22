#pragma once

#include "net/inet/ip.hpp"
#include "net/inet/port.hpp"
#include "net/sockets/base_sockaddr.hpp"

namespace core::net::inet {

/**
 * @brief IPv4 address:port pair
 */
class sockaddr final : public net::sockets::base_sockaddr {
 public:
  sockaddr(net::inet::ip ip, net::inet::port port);

 public:
  net::inet::ip get_ip() const noexcept;
  net::inet::port get_port() const noexcept;
};

}  // namespace core::net::inet

template <>
struct std::formatter<core::net::inet::sockaddr> {
  template <class FormatContext>
  constexpr auto parse(FormatContext& ctx) const noexcept {
    return ctx.begin();
  }

  template <class FormatContext>
  constexpr auto format(const core::net::inet::sockaddr& sockaddr,
                        FormatContext& ctx) const {
    return std::format_to(ctx.out(), "{}", sockaddr.to_string());
  }
};
