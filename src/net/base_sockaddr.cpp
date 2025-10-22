#include <arpa/inet.h>
#include <sys/un.h>

#include <cassert>
#include <cerrno>
#include <sandbox/net/base_sockaddr.hpp>
#include <sandbox/utils/syscall.hpp>

namespace sandbox::net {

base_sockaddr::base_sockaddr(net::family family) noexcept(false) {
  switch (family) {
    case net::family::kUnspecified:
      throw std::invalid_argument(
          std::format("invalid sockaddr family: {}", family));
    case net::family::kInet:
      storage_ = std::make_shared<::sockaddr_in>(::sockaddr_in{
          .sin_family = AF_INET,
      });
      break;
    case net::family::kInet6:
      storage_ = std::make_shared<::sockaddr_in6>(::sockaddr_in6{
          .sin6_family = AF_INET6,
      });
      break;
    case net::family::kUnix:
      storage_ = std::make_shared<::sockaddr_un>(::sockaddr_un{
          .sun_family = AF_UNIX,
      });
      break;
  }
}

base_sockaddr::storage* base_sockaddr::get_storage() noexcept {
  return static_cast<base_sockaddr::storage*>(storage_.get());
}

const base_sockaddr::storage* base_sockaddr::get_storage() const noexcept {
  return static_cast<const base_sockaddr::storage*>(storage_.get());
}

bool base_sockaddr::operator==(const base_sockaddr& that) const
    noexcept(false) {
  if (storage_ == that.storage_) {
    return true;
  }

  switch (static_cast<const ::sockaddr*>(storage_.get())->sa_family) {
    case AF_INET: {
      const ::sockaddr_in* this_sockaddr =
          static_cast<const ::sockaddr_in*>(storage_.get());
      const ::sockaddr_in* that_sockaddr =
          static_cast<const ::sockaddr_in*>(that.storage_.get());
      return !std::memcmp(&this_sockaddr->sin_addr, &that_sockaddr->sin_addr,
                          sizeof(::sockaddr_in{}.sin_addr)) &&
             this_sockaddr->sin_port == that_sockaddr->sin_port;
    }
    case AF_INET6: {
      const ::sockaddr_in6* this_sockaddr =
          static_cast<const ::sockaddr_in6*>(storage_.get());
      const ::sockaddr_in6* that_sockaddr =
          static_cast<const ::sockaddr_in6*>(that.storage_.get());
      return !std::memcmp(&this_sockaddr->sin6_addr, &that_sockaddr->sin6_addr,
                          sizeof(::sockaddr_in6{}.sin6_addr)) &&
             this_sockaddr->sin6_port == that_sockaddr->sin6_port;
    }
    case AF_UNIX: {
      const ::sockaddr_un* this_sockaddr =
          static_cast<const ::sockaddr_un*>(storage_.get());
      const ::sockaddr_un* that_sockaddr =
          static_cast<const ::sockaddr_un*>(that.storage_.get());
      return !std::strcmp(this_sockaddr->sun_path, that_sockaddr->sun_path);
    }
    [[unlikely]] default:
      throw std::runtime_error("unimplemented");
  }
}

bool base_sockaddr::operator!=(const base_sockaddr& that) const
    noexcept(false) {
  return !operator==(that);
}

std::size_t base_sockaddr::get_length() const {
  switch (static_cast<const ::sockaddr*>(storage_.get())->sa_family) {
    case AF_INET:
      return sizeof(::sockaddr_in);
    case AF_INET6:
      return sizeof(::sockaddr_in6);
    case AF_UNIX:
      return sizeof(::sockaddr_un);
    [[unlikely]] default:
      throw std::runtime_error("unimplemented");
  }
}

net::family base_sockaddr::get_family() const noexcept(false) {
  switch (static_cast<const ::sockaddr*>(storage_.get())->sa_family) {
    case AF_INET:
      return net::family::kInet;
    case AF_INET6:
      return net::family::kInet6;
    case AF_UNIX:
      return net::family::kUnix;
    [[unlikely]] default:
      throw std::runtime_error("unimplemented");
  }
}

std::string base_sockaddr::to_string() const noexcept(false) {
  constexpr std::size_t kPortStrlen = 6;
  constexpr std::size_t kColonStrlen = 1;
  constexpr std::size_t kBracketStrlen = 1;
  constexpr const char* kInetNtopError = nullptr;

  std::string string;
  switch (static_cast<const ::sockaddr*>(storage_.get())->sa_family) {
    case AF_INET: {
      const ::sockaddr_in* sockaddr =
          static_cast<const ::sockaddr_in*>(storage_.get());

      string.resize(INET_ADDRSTRLEN + kColonStrlen + kPortStrlen);
      const auto retval = SYSCALL(::inet_ntop(AF_INET, &sockaddr->sin_addr,
                                              string.data(), string.size()));
      if (retval == kInetNtopError) [[unlikely]] {
        throw std::runtime_error(
            std::format("inet_ntop failed: {}", std::strerror(errno)));
      }
      string.resize(string.find('\0'));
      string.push_back(':');
      string.append(std::to_string(ntohs(sockaddr->sin_port)));
      break;
    }
    case AF_INET6: {
      const ::sockaddr_in6* sockaddr =
          static_cast<const ::sockaddr_in6*>(storage_.get());

      string.resize(kBracketStrlen + INET6_ADDRSTRLEN + kBracketStrlen +
                    kColonStrlen + kPortStrlen);
      string.front() = '[';
      const auto retval =
          ::inet_ntop(AF_INET6, &sockaddr->sin6_addr,
                      string.data() + kBracketStrlen, string.size());
      if (retval == kInetNtopError) [[unlikely]] {
        throw std::runtime_error(
            std::format("inet_ntop failed: {}", std::strerror(errno)));
      }
      string.resize(string.find('\0'));
      string.push_back(']');
      string.push_back(':');
      string.append(std::to_string(ntohs(sockaddr->sin6_port)));
      break;
    }
    case AF_UNIX: {
      const ::sockaddr_un* sockaddr =
          static_cast<const ::sockaddr_un*>(storage_.get());

      string.assign(sockaddr->sun_path);
      break;
    }
    [[unlikely]] default:
      throw std::runtime_error("unimplemented");
  }
  return string;
}

}  // namespace sandbox::net