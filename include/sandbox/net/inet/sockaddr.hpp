#pragma once

#include <sandbox/net/base_sockaddr.hpp>
#include <sandbox/net/inet/ip.hpp>
#include <sandbox/net/inet/port.hpp>

namespace sandbox::net::inet {

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

}  // namespace sandbox::net::inet

template <>
class std::formatter<sandbox::net::inet::sockaddr> {
 public:
  template <typename FormatContext>
  constexpr auto parse(FormatContext& ctx) const noexcept {
    return ctx.begin();
  }

  template <typename FormatContext>
  constexpr auto format(const sandbox::net::inet::sockaddr& sockaddr,
                        FormatContext& ctx) const noexcept(false) {
    return std::format_to(ctx.out(), "{}", sockaddr.to_string());
  }
};
