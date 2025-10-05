#pragma once

#include <span>

#include "io/fd.hpp"
#include "net/base_sockaddr.hpp"
#include "net/sockets/family.hpp"
#include "net/sockets/protocol.hpp"
#include "net/sockets/type.hpp"

namespace core::net {

/**
 * @brief Return code for bind()
 */
enum class bind_status : std::uint8_t {
  kSuccess,
  kInUse,
};

/**
 * @brief Return code for connect()
 */
enum class connect_status : std::uint8_t {
  kSuccess,
  kPending,
  kRefused,
};

/**
 * @brief Return code for accept()
 */
enum class accept_status : std::uint8_t {
  kSuccess,
  kEmptyQueue,
};

/**
 * @brief Socket base class
 */
class base_socket : public io::fd {
 protected:
  base_socket(io::fd::uninitialized_t) noexcept;
  base_socket(net::sockets::family family, net::sockets::type type,
              net::sockets::protocol protocol);

 public:
  void set_nonblock(bool value) noexcept(false);
  void set_reuseaddr(bool value) noexcept(false);
  void set_reuseport(bool value) noexcept(false);
  void set_keepalive(bool value) noexcept(false);

 public:
  bool get_nonblock() const noexcept(false);
  bool get_reuseaddr() const noexcept(false);
  bool get_reuseport() const noexcept(false);
  bool get_keepalive() const noexcept(false);
  net::sockets::family get_family() const noexcept(false);
  net::sockets::type get_type() const noexcept(false);

 public:
  bind_status bind(const net::base_sockaddr& sockaddr) noexcept(false);

 public:
  connect_status connect(const net::base_sockaddr& sockaddr) noexcept(false);

 public:
  void get_bind_sockaddr(net::base_sockaddr& sockaddr) const noexcept(false);
  void get_connect_sockaddr(net::base_sockaddr& sockaddr) const noexcept(false);

 public:
  void listen(std::size_t backlog) noexcept(false);
  accept_status accept(base_socket& socket) const noexcept(false);

 public:
  std::size_t send(std::span<const std::byte> bytes) const noexcept(false);
  std::size_t send_to(std::span<const std::byte> bytes,
                      const net::base_sockaddr& sockaddr) const noexcept(false);
  std::size_t receive(std::span<std::byte> bytes) const noexcept(false);
  std::size_t receive_from(std::span<std::byte> bytes,
                           net::base_sockaddr& sockaddr) const noexcept(false);
};

}  // namespace core::net
