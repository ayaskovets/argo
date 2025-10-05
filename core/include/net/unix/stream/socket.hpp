#pragma once

#include "net/unix/base_socket.hpp"

namespace core::net::unix::stream {

/**
 * @brief UNIX stream socket
 */
class socket final : public net::unix::base_socket {
 public:
  socket(io::fd::uninitialized_t) noexcept;
  socket() noexcept(false);
};

}  // namespace core::net::unix::stream
