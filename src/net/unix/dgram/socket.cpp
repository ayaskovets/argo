#include <wip/net/unix/dgram/socket.hpp>

namespace wip::net::unix::dgram {

socket::socket(io::uninitialized_t) noexcept
    : net::unix::base_socket(wip::io::uninitialized_t{}) {}

socket::socket() noexcept(false)
    : net::unix::base_socket(net::socktype::kDgram) {}

}  // namespace wip::net::unix::dgram
