#pragma once

#include <string_view>
#include <variant>
#include <vector>

#include "net/inet/sockaddr.hpp"
#include "net/inet6/sockaddr.hpp"
#include "net/sockets/protocol.hpp"

namespace core::net::dns {

/**
 * @brief Resolve IPv4 or IPv6 address from a hostname
 *
 * Use family::kUnspecified to resolve all available addresses
 *
 * Use protocol::kUnspecified to resolve all available protocols
 */
std::vector<std::variant<net::inet::sockaddr, net::inet6::sockaddr>> resolve(
    std::string_view hostname, net::sockets::family family,
    net::sockets::protocol protocol) noexcept(false);

}  // namespace core::net::dns
