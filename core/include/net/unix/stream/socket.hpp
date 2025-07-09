#pragma once

#include "net/unix/base_socket.hpp"

namespace core::net::unix::stream {

class socket final : public net::unix::base_socket {
 public:
  socket(io::fd::uninitialized_t) noexcept;
  socket();
};

}  // namespace core::net::unix::stream
