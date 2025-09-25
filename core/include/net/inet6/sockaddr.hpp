#pragma once

#include "net/base_sockaddr.hpp"
#include "net/inet6/ip.hpp"
#include "net/inet6/port.hpp"

namespace core::net::inet6 {

/**
 * @brief IPv6 address:port pair
 */
class sockaddr final : public net::base_sockaddr {
 public:
  sockaddr(net::inet6::ip ip, net::inet6::port port);

 public:
  net::inet6::ip get_ip() const noexcept;
  net::inet6::port get_port() const noexcept;
};

}  // namespace core::net::inet6

template <>
struct std::formatter<core::net::inet6::sockaddr> {
  template <typename FormatContext>
  constexpr auto parse(FormatContext& ctx) const noexcept {
    return ctx.begin();
  }

  template <typename FormatContext>
  constexpr auto format(const core::net::inet6::sockaddr& sockaddr,
                        FormatContext& ctx) const {
    return std::format_to(ctx.out(), "{}", sockaddr.to_string());
  }
};
