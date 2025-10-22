#include <sandbox/net/unix/dgram/socket.hpp>

namespace sandbox::net::unix::dgram {

socket::socket(io::uninitialized_t) noexcept
    : net::unix::base_socket(sandbox::io::uninitialized_t{}) {}

socket::socket() noexcept(false)
    : net::unix::base_socket(net::socktype::kDgram) {}

}  // namespace sandbox::net::unix::dgram
