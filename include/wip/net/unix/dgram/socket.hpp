#pragma once

#include <wip/net/unix/base_socket.hpp>

namespace wip::net::unix::dgram {

/**
 * @brief UNIX datagram socket
 */
class socket final : public net::unix::base_socket {
 public:
  socket(io::uninitialized_t) noexcept;
  socket() noexcept(false);
};

}  // namespace wip::net::unix::dgram
