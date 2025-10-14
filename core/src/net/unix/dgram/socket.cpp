#include "net/unix/dgram/socket.hpp"

namespace core::net::unix::dgram {

socket::socket(io::uninitialized_t) noexcept
    : net::unix::base_socket(core::io::uninitialized_t{}) {}

socket::socket() noexcept(false)
    : net::unix::base_socket(net::sockets::type::kDgram) {}

}  // namespace core::net::unix::dgram
