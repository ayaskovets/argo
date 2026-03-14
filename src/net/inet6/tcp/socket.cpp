#include <wip/net/inet6/tcp/socket.hpp>

namespace wip::net::inet6::tcp {

socket::socket(io::uninitialized_t) noexcept
    : net::base_socket(wip::io::uninitialized_t{}) {}

socket::socket() noexcept(false)
    : net::base_socket(net::family::kInet6, net::socktype::kStream,
                       net::protocol::kTcp) {}

}  // namespace wip::net::inet6::tcp
