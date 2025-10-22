#pragma once

#include <sandbox/net/unix/base_socket.hpp>

namespace sandbox::net::unix::stream {

/**
 * @brief UNIX stream socket
 */
class socket final : public net::unix::base_socket {
 public:
  socket(io::uninitialized_t) noexcept;
  socket() noexcept(false);
};

}  // namespace sandbox::net::unix::stream
