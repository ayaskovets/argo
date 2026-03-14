#include <wip/net/base_sockaddr.hpp>
#include <wip/utils/syscall.hpp>

#include <cassert>
#include <cerrno>

#include <arpa/inet.h>
#include <sys/un.h>

namespace wip::net {

base_sockaddr::base_sockaddr(net::family family) noexcept(false)
    : impl_([family] {
        switch (family) {
          case net::family::kUnspecified:
            throw std::invalid_argument(
                std::format("invalid sockaddr family: {}", family));
          case net::family::kInet:
            return utils::unchecked_any(::sockaddr_in{.sin_family = AF_INET});
          case net::family::kInet6:
            return utils::unchecked_any(
                ::sockaddr_in6{.sin6_family = AF_INET6});
          case net::family::kUnix:
            return utils::unchecked_any(::sockaddr_un{.sun_family = AF_UNIX});
        }
      }()) {}

base_sockaddr::storage* base_sockaddr::get_storage() noexcept {
  return &impl_.get<base_sockaddr::storage>();
}

const base_sockaddr::storage* base_sockaddr::get_storage() const noexcept {
  return &impl_.get<base_sockaddr::storage>();
}

bool base_sockaddr::operator==(const base_sockaddr& that) const
    noexcept(false) {
  switch (impl_.get<::sockaddr>().sa_family) {
    case AF_INET: {
      const auto& this_sockaddr = impl_.get<::sockaddr_in>();
      const auto& that_sockaddr = that.impl_.get<::sockaddr_in>();
      return !std::memcmp(&this_sockaddr.sin_addr, &that_sockaddr.sin_addr,
                          sizeof(::sockaddr_in{}.sin_addr)) &&
             this_sockaddr.sin_port == that_sockaddr.sin_port;
    }
    case AF_INET6: {
      const auto& this_sockaddr = impl_.get<::sockaddr_in6>();
      const auto& that_sockaddr = that.impl_.get<::sockaddr_in6>();
      return !std::memcmp(&this_sockaddr.sin6_addr, &that_sockaddr.sin6_addr,
                          sizeof(::sockaddr_in6{}.sin6_addr)) &&
             this_sockaddr.sin6_port == that_sockaddr.sin6_port;
    }
    case AF_UNIX: {
      const auto& this_sockaddr = impl_.get<::sockaddr_un>();
      const auto& that_sockaddr = that.impl_.get<::sockaddr_un>();
      return !std::strcmp(this_sockaddr.sun_path, that_sockaddr.sun_path);
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
  switch (impl_.get<::sockaddr>().sa_family) {
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
  switch (impl_.get<::sockaddr>().sa_family) {
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
  switch (impl_.get<::sockaddr>().sa_family) {
    case AF_INET: {
      const auto& sockaddr = impl_.get<::sockaddr_in>();

      string.resize(INET_ADDRSTRLEN + kColonStrlen + kPortStrlen);
      const auto retval = SYSCALL(::inet_ntop(AF_INET, &sockaddr.sin_addr,
                                              string.data(), string.size()));
      if (retval == kInetNtopError) [[unlikely]] {
        throw std::runtime_error(
            std::format("inet_ntop failed: {}", std::strerror(errno)));
      }
      string.resize(string.find('\0'));
      string.push_back(':');
      string.append(std::to_string(ntohs(sockaddr.sin_port)));
      break;
    }
    case AF_INET6: {
      const auto& sockaddr = impl_.get<::sockaddr_in6>();

      string.resize(kBracketStrlen + INET6_ADDRSTRLEN + kBracketStrlen +
                    kColonStrlen + kPortStrlen);
      string.front() = '[';
      const auto retval =
          ::inet_ntop(AF_INET6, &sockaddr.sin6_addr,
                      string.data() + kBracketStrlen, string.size());
      if (retval == kInetNtopError) [[unlikely]] {
        throw std::runtime_error(
            std::format("inet_ntop failed: {}", std::strerror(errno)));
      }
      string.resize(string.find('\0'));
      string.push_back(']');
      string.push_back(':');
      string.append(std::to_string(ntohs(sockaddr.sin6_port)));
      break;
    }
    case AF_UNIX: {
      const auto& sockaddr = impl_.get<::sockaddr_un>();

      string.assign(sockaddr.sun_path);
      break;
    }
    [[unlikely]] default:
      throw std::runtime_error("unimplemented");
  }
  return string;
}

}  // namespace wip::net