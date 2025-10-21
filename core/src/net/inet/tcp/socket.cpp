#include "net/inet/tcp/socket.hpp"

namespace core::net::inet::tcp {

socket::socket(io::uninitialized_t) noexcept
    : net::base_socket(io::uninitialized_t{}) {}

socket::socket() noexcept(false)
    : net::base_socket(net::family::kInet, net::socktype::kStream,
                       net::protocol::kTcp) {}

}  // namespace core::net::inet::tcp
