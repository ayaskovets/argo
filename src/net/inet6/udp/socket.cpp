#include <wip/net/inet6/udp/socket.hpp>

namespace wip::net::inet6::udp {

socket::socket(io::uninitialized_t) noexcept
    : net::base_socket(wip::io::uninitialized_t{}) {}

socket::socket() noexcept(false)
    : net::base_socket(net::family::kInet6, net::socktype::kDgram,
                       net::protocol::kUdp) {}

}  // namespace wip::net::inet6::udp
