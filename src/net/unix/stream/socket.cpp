#include <sandbox/net/unix/stream/socket.hpp>

namespace sandbox::net::unix::stream {

socket::socket(io::uninitialized_t) noexcept
    : net::unix::base_socket(sandbox::io::uninitialized_t{}) {}

socket::socket() noexcept(false)
    : net::unix::base_socket(net::socktype::kStream) {}

}  // namespace sandbox::net::unix::stream
