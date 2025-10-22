#include <sys/un.h>

#include <sandbox/net/unix/sockaddr.hpp>

namespace sandbox::net::unix {

const sockaddr& sockaddr::kInvalid() noexcept {
  static const sockaddr sockaddr("/");
  return sockaddr;
}

const sockaddr& sockaddr::kEmpty() noexcept {
  static const sockaddr sockaddr("");
  return sockaddr;
}

sockaddr::sockaddr(std::string_view path) noexcept(false)
    : net::base_sockaddr(net::family::kUnix) {
  constexpr std::size_t kMaxPathSize = sizeof(sockaddr_un{}.sun_path) - 1;

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

}  // namespace sandbox::net::unix
