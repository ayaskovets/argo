#pragma once

#include <wip/net/unix/base_socket.hpp>

namespace wip::net::unix::stream {

/**
 * @brief UNIX stream socket
 */
class socket final : public net::unix::base_socket {
 public:
  socket(io::uninitialized_t) noexcept;
  socket() noexcept(false);
};

}  // namespace wip::net::unix::stream
