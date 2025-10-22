#pragma once

#include <sandbox/io/fd.hpp>
#include <sandbox/io/poller.hpp>
#include <sandbox/net/base_socket.hpp>
#include <sandbox/utils/mixins.hpp>

#include <sandbox/async/executors/current_executor.hpp"
#include <sandbox/async/schedulers/io_scheduler.hpp"

namespace sandbox::async::awaiters {

enum class action {
  kNone,
  kAccept,
  kConnect,
};

class io_awaiter final : public utils::non_copyable, public utils::non_movable {
 public:
  constexpr explicit io_awaiter(net::base_socket& socket,
                                net::base_socket* peer,
                                const net::base_sockaddr* sockaddr,
                                const io::poller::events_t events) noexcept
      : params_(std::make_unique<schedulers::detail::io_scheduler_input<
                    std::coroutine_handle<>>>(socket, events, nullptr)) {
    socket_ = &socket;
    if (peer) {
      peer_ = peer;
      action_ = action::kAccept;
    }
    if (sockaddr) {
      sockaddr_ = sockaddr;
      action_ = action::kConnect;
    }
  }

  constexpr io_awaiter() noexcept : action_(action::kNone), params_(nullptr) {}

 public:
  constexpr auto await_ready() { return action_ == action::kNone; }

  template <typename Promise>
  auto await_suspend(std::coroutine_handle<Promise> parent) {
    params_ = std::make_unique<
        schedulers::detail::io_scheduler_input<std::coroutine_handle<>>>(
        *socket_, params_->events(), parent);
    executors::current_executor::suspend(*params_);
  }

  constexpr auto await_resume() {
    switch (action_) {
      case action::kNone:
        break;
      case action::kAccept:
        if (net::accept_status::kSuccess != socket_->accept(*peer_)) {
          throw std::runtime_error("what (2)");
        }
        break;
      case action::kConnect:
        if (net::connect_status::kSuccess != socket_->connect(*sockaddr_)) {
          throw std::runtime_error("what (2)");
        }
        break;
    }
  }

 private:
  action action_;
  std::unique_ptr<
      schedulers::detail::io_scheduler_input<std::coroutine_handle<>>>
      params_;
  net::base_socket* socket_;
  net::base_socket* peer_;
  const net::base_sockaddr* sockaddr_;
};

constexpr auto accept(net::base_socket& socket, net::base_socket& peer) {
  if (net::accept_status::kSuccess == socket.accept(peer)) {
    return io_awaiter();
  }
  return io_awaiter(socket, &peer, nullptr, io::poller::event::kPollIn);
}

constexpr auto connect(net::base_socket& socket,
                       const net::base_sockaddr& sockaddr) {
  if (net::connect_status::kSuccess == socket.connect(sockaddr)) {
    return io_awaiter();
  }
  return io_awaiter(socket, nullptr, &sockaddr, io::poller::event::kPollIn);
}

constexpr auto receive(net::base_socket& socket, std::span<std::byte> data) {
  if (socket.receive(data) != 0) {
    return io_awaiter();
  }
  return io_awaiter(socket, nullptr, nullptr, io::poller::event::kPollIn);
}

constexpr auto send(net::base_socket& socket, std::span<std::byte> data) {
  if (socket.send(data) == data.size()) {
    return io_awaiter();
  }
  return io_awaiter(socket, nullptr, nullptr, io::poller::event::kPollOut);
}

}  // namespace sandbox::async::awaiters
