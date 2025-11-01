#include <sandbox/net/unix/sockaddr.hpp>

#include <sys/un.h>

namespace sandbox::net::unix {

const sockaddr& sockaddr::kInvalid() noexcept {
  static const sockaddr sockaddr("/");
  return sockaddr;
}

const sockaddr& sockaddr::kEmpty() noexcept {
  static const sockaddr sockaddr("");
  return sockaddr;
}

sockaddr::sockaddr(const std::filesystem::path& path) noexcept(false)
    : net::base_sockaddr(net::family::kUnix) {
  constexpr std::size_t kMaxPathSize = sizeof(sockaddr_un{}.sun_path) - 1;

  if (path.native().size() >= kMaxPathSize) [[unlikely]] {
    throw std::invalid_argument(
        std::format("invalid path size {}", path.native().size()));
  }

  ::sockaddr_un* storage = reinterpret_cast<::sockaddr_un*>(get_storage());
  std::strncpy(storage->sun_path, path.c_str(), path.native().size());
}

std::filesystem::path sockaddr::get_path() const noexcept {
  const ::sockaddr_un* storage =
      reinterpret_cast<const ::sockaddr_un*>(get_storage());
  return std::filesystem::path(storage->sun_path);
}

}  // namespace sandbox::net::unix
