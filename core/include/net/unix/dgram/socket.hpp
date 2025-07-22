#pragma once

#include "net/unix/base_socket.hpp"

namespace core::net::unix::dgram {

/**
 * @brief UNIX datagram socket
 */
class socket final : public net::unix::base_socket {
 public:
  socket(io::fd::uninitialized_t) noexcept;
  socket();
};

}  // namespace core::net::unix::dgram
