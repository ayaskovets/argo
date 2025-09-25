#pragma once

#include "net/base_socket.hpp"
#include "net/unix/sockaddr.hpp"

namespace core::net::unix {

/**
 * @brief UNIX socket base class
 */
class base_socket : public net::base_socket {
 protected:
  explicit base_socket(io::fd::uninitialized_t) noexcept;
  explicit base_socket(core::net::sockets::type type);
  ~base_socket() noexcept;

 public:
  void unlink_and_close();

 public:
  net::bind_status unlink_bind(const net::unix::sockaddr& sockaddr);
};

}  // namespace core::net::unix
