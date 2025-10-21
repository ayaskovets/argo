#include "net/inet6/tcp/socket.hpp"

namespace core::net::inet6::tcp {

socket::socket(io::uninitialized_t) noexcept
    : net::base_socket(core::io::uninitialized_t{}) {}

socket::socket() noexcept(false)
    : net::base_socket(net::family::kInet6, net::socktype::kStream,
                       net::protocol::kTcp) {}

}  // namespace core::net::inet6::tcp
