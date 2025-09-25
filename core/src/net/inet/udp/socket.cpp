#include "net/inet/udp/socket.hpp"

namespace core::net::inet::udp {

socket::socket(io::fd::uninitialized_t) noexcept
    : net::base_socket(core::io::fd::uninitialized_t{}) {}

socket::socket()
    : net::base_socket(net::sockets::family::kInet, net::sockets::type::kDgram,
                       net::sockets::protocol::kUdp) {}

}  // namespace core::net::inet::udp
