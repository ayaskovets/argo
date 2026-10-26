#pragma once

#include <skynet/base_socket.hpp>
#include <skynet/multiplexing.hpp>

#include "../execution_token.hpp"

namespace argo::schedulers {

/**
 * @brief Input channel value type of the network scheduler
 */
class network_in_value final {
 public:
  constexpr network_in_value(argo::execution_token token,
                             skynet::base_socket const & socket,
                             skynet::reactor::event_filter filter) noexcept
      : execution_token_(std::move(token)),
        socket_(&socket),
        filter_(filter),
        revents_() {}

 public:
  constexpr auto get_execution_token() noexcept -> argo::execution_token && {
    return std::move(execution_token_);
  }

  constexpr auto get_socket() const noexcept -> skynet::base_socket const & {
    return *socket_;
  }

  constexpr auto get_filter() const noexcept -> skynet::reactor::event_filter {
    return filter_;
  }

  constexpr auto get_revents() const noexcept -> skynet::reactor::event_filter {
    return revents_;
  }

 public:
  constexpr void set_revents(skynet::reactor::event_filter value) noexcept {
    revents_ = value;
  }

 private:
  argo::execution_token execution_token_;
  skynet::base_socket const * socket_;
  skynet::reactor::event_filter filter_;
  // NOLINTNEXTLINE(cppcoreguidelines-use-default-member-init)
  skynet::reactor::event_filter revents_;
};

}  // namespace argo::schedulers
