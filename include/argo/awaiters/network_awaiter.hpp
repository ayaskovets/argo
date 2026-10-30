#pragma once

#include <skynet/base_sockaddr.hpp>
#include <skynet/base_socket.hpp>
#include <skynet/multiplexing.hpp>

#include "../execution_context.hpp"
#include "../execution_token.hpp"
#include "../schedulers/network_in_value.hpp"

namespace argo::awaiters {

/**
 * @brief Suspend execution until some network event occurs on a given socket
 * @warning The socket is expected to be nonblocking
 */
template <typename Socket, typename Operation>
class network final {
 public:
  constexpr network(Socket & socket, skynet::reactor::event_filter filter,
                    Operation operation) noexcept
      : socket_(&socket), filter_(filter), operation_(std::move(operation)) {}

 public:
  constexpr auto operator co_await() && noexcept -> auto {
    class network_awaiter final {
     public:
      constexpr network_awaiter(Socket & socket,
                                skynet::reactor::event_filter filter,
                                Operation operation) noexcept
          : in_value_({}, socket, filter), operation_(std::move(operation)) {}

     public:
      constexpr auto await_ready() -> bool { return operation_.await_ready(); }

      constexpr void await_suspend(execution_token token) {
        in_value_ = {std::move(token), in_value_.get_socket(),
                     in_value_.get_filter()};
        execution_context::this_executor().schedule(&in_value_);
      }

      constexpr auto await_resume() -> decltype(auto) {
        return operation_.await_resume(in_value_.get_revents());
      }

     private:
      schedulers::network_in_value in_value_;
      Operation operation_;
    };

    return network_awaiter(*socket_, filter_, std::move(operation_));
  }

 private:
  Socket * socket_;
  skynet::reactor::event_filter filter_;
  Operation operation_;
};

}  // namespace argo::awaiters

/**
 * @section Utility constructors
 */
namespace argo::awaiters {

template <typename Socket>
constexpr auto accept(Socket & socket, Socket & peer) noexcept -> auto {
  class operation final {
   public:
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    constexpr operation(Socket & socket, Socket & peer) noexcept
        : socket_(&socket),
          peer_(&peer),
          status_(skynet::accept_status::kEmptyQueue) {}

   public:
    constexpr auto await_ready() -> bool {
      status_ = socket_->accept(*peer_);
      return status_ == skynet::accept_status::kSuccess;
    }

    constexpr auto await_resume(skynet::reactor::event_filter)
        -> skynet::accept_status {
      if (status_ == skynet::accept_status::kEmptyQueue) {
        status_ = socket_->accept(*peer_);
      }
      return status_;
    }

   private:
    Socket * socket_;
    Socket * peer_;
    skynet::accept_status status_;
  };

  return network(socket, skynet::reactor::event_filter::kPollIn,
                 operation(socket, peer));
}

template <typename Socket>
constexpr auto connect(Socket & socket, skynet::base_sockaddr const & address)
    -> auto {
  class operation final {
   public:
    constexpr operation(Socket & socket,
                        skynet::base_sockaddr const & address) noexcept
        : socket_(&socket),
          address_(&address),
          status_(skynet::connect_status::kPending) {}

   public:
    constexpr auto await_ready() -> bool {
      status_ = socket_->connect(*address_);
      return status_ != skynet::connect_status::kPending;
    }

    constexpr auto await_resume(skynet::reactor::event_filter revents)
        -> skynet::connect_status {
      if (status_ == skynet::connect_status::kPending) {
        status_ = (revents & skynet::reactor::kPollErr) != 0
                      ? skynet::connect_status::kRefused
                      : skynet::connect_status::kSuccess;
      }
      return status_;
    }

   private:
    Socket * socket_;
    skynet::base_sockaddr const * address_;
    skynet::connect_status status_;
  };

  return network(socket, skynet::reactor::event_filter::kPollOut,
                 operation(socket, address));
}

template <typename Socket>
constexpr auto send(Socket & socket, std::span<std::byte const> bytes) noexcept
    -> auto {
  class operation final {
   public:
    constexpr operation(Socket & socket,
                        std::span<std::byte const> bytes) noexcept
        : socket_(&socket),
          bytes_(bytes),
          size_(skynet::base_socket::kWouldBlock) {}

   public:
    constexpr auto await_ready() -> bool {
      size_ = socket_->send(bytes_);
      return size_ != skynet::base_socket::kWouldBlock;
    }

    constexpr auto await_resume(skynet::reactor::event_filter) -> std::size_t {
      if (size_ == skynet::base_socket::kWouldBlock) {
        size_ = socket_->send(bytes_);
      }
      return size_;
    }

   private:
    Socket * socket_;
    std::span<std::byte const> bytes_;
    std::size_t size_;
  };

  return network(socket, skynet::reactor::event_filter::kPollOut,
                 operation(socket, bytes));
}

template <typename Socket>
constexpr auto sendto(Socket & socket, std::span<std::byte const> bytes,
                      skynet::base_sockaddr const & address) noexcept -> auto {
  class operation final {
   public:
    constexpr operation(Socket & socket, std::span<std::byte const> bytes,
                        skynet::base_sockaddr const & address) noexcept
        : socket_(&socket),
          bytes_(bytes),
          address_(&address),
          size_(skynet::base_socket::kWouldBlock) {}

   public:
    constexpr auto await_ready() -> bool {
      size_ = socket_->sendto(bytes_, *address_);
      return size_ != skynet::base_socket::kWouldBlock;
    }

    constexpr auto await_resume(skynet::reactor::event_filter) -> std::size_t {
      if (size_ == skynet::base_socket::kWouldBlock) {
        size_ = socket_->sendto(bytes_, *address_);
      }
      return size_;
    }

   private:
    Socket * socket_;
    std::span<std::byte const> bytes_;
    skynet::base_sockaddr const * address_;
    std::size_t size_;
  };

  return network(socket, skynet::reactor::event_filter::kPollOut,
                 operation(socket, bytes, address));
}

template <typename Socket>
constexpr auto recv(Socket & socket, std::span<std::byte> bytes) noexcept
    -> auto {
  class operation final {
   public:
    constexpr operation(Socket & socket, std::span<std::byte> bytes) noexcept
        : socket_(&socket),
          bytes_(bytes),
          size_(skynet::base_socket::kWouldBlock) {}

   public:
    constexpr auto await_ready() -> bool {
      size_ = socket_->recv(bytes_);
      return size_ != skynet::base_socket::kWouldBlock;
    }

    constexpr auto await_resume(skynet::reactor::event_filter) -> std::size_t {
      if (size_ == skynet::base_socket::kWouldBlock) {
        size_ = socket_->recv(bytes_);
      }
      return size_;
    }

   private:
    Socket * socket_;
    std::span<std::byte> bytes_;
    std::size_t size_;
  };

  return network(socket, skynet::reactor::event_filter::kPollIn,
                 operation(socket, bytes));
}

template <typename Socket>
constexpr auto recvfrom(Socket & socket, std::span<std::byte> bytes,
                        skynet::base_sockaddr & address) noexcept -> auto {
  class operation final {
   public:
    constexpr operation(Socket & socket, std::span<std::byte> bytes,
                        skynet::base_sockaddr & address) noexcept
        : socket_(&socket),
          bytes_(bytes),
          address_(&address),
          size_(skynet::base_socket::kWouldBlock) {}

   public:
    constexpr auto await_ready() -> bool {
      size_ = socket_->recvfrom(bytes_, *address_);
      return size_ != skynet::base_socket::kWouldBlock;
    }

    constexpr auto await_resume(skynet::reactor::event_filter) -> std::size_t {
      if (size_ == skynet::base_socket::kWouldBlock) {
        size_ = socket_->recvfrom(bytes_, *address_);
      }
      return size_;
    }

   private:
    Socket * socket_;
    std::span<std::byte> bytes_;
    skynet::base_sockaddr * address_;
    std::size_t size_;
  };

  return network(socket, skynet::reactor::event_filter::kPollIn,
                 operation(socket, bytes, address));
}

}  // namespace argo::awaiters
