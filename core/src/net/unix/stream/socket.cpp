#include "net/unix/stream/socket.hpp"

namespace core::net::unix::stream {

socket::socket(io::uninitialized_t) noexcept
    : net::unix::base_socket(core::io::uninitialized_t{}) {}

socket::socket() noexcept(false)
    : net::unix::base_socket(net::socktype::kStream) {}

}  // namespace core::net::unix::stream
