#include "net/unix/stream/socket.hpp"

namespace core::net::unix::stream {

socket::socket(io::fd::uninitialized_t) noexcept
    : net::unix::base_socket(core::io::fd::uninitialized_t{}) {}

socket::socket() noexcept(false)
    : net::unix::base_socket(net::sockets::type::kStream) {}

}  // namespace core::net::unix::stream
