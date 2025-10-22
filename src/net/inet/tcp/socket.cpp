#include <sandbox/net/inet/tcp/socket.hpp>

namespace sandbox::net::inet::tcp {

socket::socket(io::uninitialized_t) noexcept
    : net::base_socket(io::uninitialized_t{}) {}

socket::socket() noexcept(false)
    : net::base_socket(net::family::kInet, net::socktype::kStream,
                       net::protocol::kTcp) {}

}  // namespace sandbox::net::inet::tcp
