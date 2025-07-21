#pragma once

#include <string_view>
#include <variant>
#include <vector>

#include "net/inet/sockaddr.hpp"
#include "net/inet6/sockaddr.hpp"
#include "net/sockets/protocol.hpp"

namespace core::net::dns {

std::vector<std::variant<net::inet::sockaddr, net::inet6::sockaddr>> resolve(
    std::string_view hostname, net::sockets::family family,
    net::sockets::protocol protocol);

}  // namespace core::net::dns
