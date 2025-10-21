#include "net/inet6/udp/socket.hpp"

namespace core::net::inet6::udp {

socket::socket(io::uninitialized_t) noexcept
    : net::base_socket(core::io::uninitialized_t{}) {}

socket::socket() noexcept(false)
    : net::base_socket(net::family::kInet6, net::socktype::kDgram,
                       net::protocol::kUdp) {}

}  // namespace core::net::inet6::udp
