#include <wip/net/inet/udp/socket.hpp>

namespace wip::net::inet::udp {

socket::socket(io::uninitialized_t) noexcept
    : net::base_socket(wip::io::uninitialized_t{}) {}

socket::socket() noexcept(false)
    : net::base_socket(net::family::kInet, net::socktype::kDgram,
                       net::protocol::kUdp) {}

}  // namespace wip::net::inet::udp
