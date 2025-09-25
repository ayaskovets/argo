#include "net/unix/sockaddr.hpp"

#include <sys/un.h>

namespace core::net::unix {

namespace {

constexpr std::size_t kMaxPathSize = sizeof(sockaddr_un{}.sun_path) - 1;

}  // namespace

const sockaddr& sockaddr::kInvalid() noexcept {
  static const sockaddr sockaddr("/");
  return sockaddr;
}

const sockaddr& sockaddr::kEmpty() noexcept {
  static const sockaddr sockaddr("");
  return sockaddr;
}

sockaddr::sockaddr(std::string_view path)
    : net::base_sockaddr(net::sockets::family::kUnix) {
  if (path.size() >= kMaxPathSize) [[unlikely]] {
    throw std::invalid_argument(
        std::format("invalid path size {}", path.size()));
  }

  ::sockaddr_un* storage = reinterpret_cast<::sockaddr_un*>(get_storage());
  std::strncpy(storage->sun_path, path.data(), path.size());
}

std::string_view sockaddr::get_path() const noexcept {
  const ::sockaddr_un* storage =
      reinterpret_cast<const ::sockaddr_un*>(get_storage());
  return std::string_view(storage->sun_path);
}

}  // namespace core::net::unix
