#pragma once

#include <skynet/base_sockaddr.hpp>
#include <skynet/base_socket.hpp>
#include <skynet/multiplexing.hpp>
#include <skynet/status.hpp>
#include <skynet/uninitialized.hpp>

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
      constexpr auto await_ready() -> bool {
        return (result_ = operation_(in_value_.get_revents())).has_value();
      }

      constexpr void await_suspend(execution_token token) {
        in_value_ = {std::move(token), in_value_.get_socket(),
                     in_value_.get_filter()};
        execution_context::this_executor().schedule(&in_value_);
      }

      constexpr auto await_resume() -> auto {
        auto result = result_.has_value()
                          ? std::move(*result_)
                          : std::move(operation_)(in_value_.get_revents());
        if (!result.has_value()) {
          throw std::runtime_error("network::await_resume(): not ready");
        }
        return std::move(*result);
      }

     private:
      schedulers::network_in_value in_value_;
      Operation operation_;
      std::invoke_result_t<Operation, skynet::reactor::event_filter> result_;
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
  auto operation = [&socket, &peer,
                    retry = false](skynet::reactor::event_filter) mutable
      -> std::optional<skynet::accept_status> {
    auto const status = socket.accept(peer);
    if (status == skynet::accept_status::kEmptyQueue &&
        !std::exchange(retry, true)) {
      return std::nullopt;
    }
    return status;
  };

  return network(socket, skynet::reactor::event_filter::kPollIn,
                 std::move(operation));
}

template <typename Socket>
constexpr auto connect(Socket & socket, skynet::base_sockaddr const & address)
    -> auto {
  auto operation = [&socket, address = &address, started = false](
                       skynet::reactor::event_filter revents) mutable
      -> std::optional<skynet::connect_status> {
    if (std::exchange(started, true)) {
      return (revents & skynet::reactor::kPollErr) != 0
                 ? skynet::connect_status::kRefused
                 : skynet::connect_status::kSuccess;
    }
    auto const status = socket.connect(*address);
    if (status == skynet::connect_status::kPending) {
      return std::nullopt;
    }
    return status;
  };

  return network(socket, skynet::reactor::event_filter::kPollOut,
                 std::move(operation));
}

template <typename Socket>
constexpr auto send(Socket & socket, std::span<std::byte const> bytes) noexcept
    -> auto {
  auto operation =
      [&socket, bytes, retry = false](
          skynet::reactor::event_filter) mutable -> std::optional<std::size_t> {
    auto const size = socket.send(bytes);
    if (size == skynet::base_socket::kWouldBlock &&
        !std::exchange(retry, true)) {
      return std::nullopt;
    }
    return size;
  };

  return network(socket, skynet::reactor::event_filter::kPollOut,
                 std::move(operation));
}

template <typename Socket>
constexpr auto sendto(Socket & socket, std::span<std::byte const> bytes,
                      skynet::base_sockaddr const & address) noexcept -> auto {
  auto operation =
      [&socket, bytes, address = &address, retry = false](
          skynet::reactor::event_filter) mutable -> std::optional<std::size_t> {
    auto const size = socket.sendto(bytes, *address);
    if (size == skynet::base_socket::kWouldBlock &&
        !std::exchange(retry, true)) {
      return std::nullopt;
    }
    return size;
  };

  return network(socket, skynet::reactor::event_filter::kPollOut,
                 std::move(operation));
}

template <typename Socket>
constexpr auto recv(Socket & socket, std::span<std::byte> bytes) noexcept
    -> auto {
  auto operation =
      [&socket, bytes, retry = false](
          skynet::reactor::event_filter) mutable -> std::optional<std::size_t> {
    auto const size = socket.recv(bytes);
    if (size == skynet::base_socket::kWouldBlock &&
        !std::exchange(retry, true)) {
      return std::nullopt;
    }
    return size;
  };

  return network(socket, skynet::reactor::event_filter::kPollIn,
                 std::move(operation));
}

template <typename Socket>
constexpr auto recvfrom(Socket & socket, std::span<std::byte> bytes,
                        skynet::base_sockaddr & address) noexcept -> auto {
  auto operation =
      [&socket, bytes, address = &address, retry = false](
          skynet::reactor::event_filter) mutable -> std::optional<std::size_t> {
    auto const size = socket.recvfrom(bytes, *address);
    if (size == skynet::base_socket::kWouldBlock &&
        !std::exchange(retry, true)) {
      return std::nullopt;
    }
    return size;
  };

  return network(socket, skynet::reactor::event_filter::kPollIn,
                 std::move(operation));
}

}  // namespace argo::awaiters
