#pragma once

#include <format>
#include <memory>
#include <string>

#include "net/sockets/family.hpp"

namespace core::net {

class base_socket;

/**
 * @brief Sockaddr of any family
 *
 * @warning Allocates and deallocated memory that holds sockaddr of the provided
 * family. It is the responsibility of the derived class to fill the memory as
 * needed
 */
class base_sockaddr {
 private:
  friend class base_socket;  ///< Needed for base_socket to call get_storage()

 protected:
  explicit base_sockaddr(net::sockets::family family) noexcept(false);

 protected:
  /**
   * @brief Type-erased sockaddr storage
   *
   * Allocated by the base_sockaddr. Expected to be used only in the subclasses
   * for wrapping the access to allocated storage or for two-step creation
   * of base_sockaddr object
   */
  class storage;
  storage* get_storage() noexcept;
  const storage* get_storage() const noexcept;

 public:
  bool operator==(const base_sockaddr& that) const noexcept(false);
  bool operator!=(const base_sockaddr& that) const noexcept(false);

 public:
  std::size_t get_length() const noexcept(false);
  net::sockets::family get_family() const noexcept(false);

 public:
  std::string to_string() const noexcept(false);

 private:
  std::shared_ptr<void> storage_;
};

}  // namespace core::net

template <>
class std::formatter<core::net::base_sockaddr> {
 public:
  template <typename FormatContext>
  constexpr auto parse(FormatContext& ctx) const noexcept {
    return ctx.begin();
  }

  template <typename FormatContext>
  constexpr auto format(const core::net::base_sockaddr& sockaddr,
                        FormatContext& ctx) const noexcept(false) {
    return std::format_to(ctx.out(), "{}", sockaddr.to_string());
  }
};
