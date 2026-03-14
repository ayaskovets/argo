#include <wip/net/unix/stream/socket.hpp>

namespace wip::net::unix::stream {

socket::socket(io::uninitialized_t) noexcept
    : net::unix::base_socket(wip::io::uninitialized_t{}) {}

socket::socket() noexcept(false)
    : net::unix::base_socket(net::socktype::kStream) {}

}  // namespace wip::net::unix::stream
