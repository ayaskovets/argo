#pragma once

#include <sandbox/net/inet/sockaddr.hpp>
#include <sandbox/net/inet6/sockaddr.hpp>
#include <sandbox/net/protocol.hpp>
#include <string_view>
#include <variant>
#include <vector>

namespace sandbox::net {

/**
 * @brief Resolve IPv4 or IPv6 address from a hostname
 *
 * Use family::kUnspecified to resolve all available addresses
 *
 * Use protocol::kUnspecified to resolve all available protocols
 */
std::vector<std::variant<net::inet::sockaddr, net::inet6::sockaddr>> resolve(
    std::string_view hostname, net::family family,
    net::protocol protocol) noexcept(false);

}  // namespace sandbox::net
