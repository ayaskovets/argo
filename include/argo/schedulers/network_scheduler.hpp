#pragma once

#include <cassert>

#include <skynet/base_socket.hpp>
#include <skynet/multiplexing.hpp>
#include <swiss/mixins.hpp>

#include "../channel.hpp"
#include "../execution_token.hpp"

namespace argo::schedulers::concepts {

template <typename T>
concept network_scheduler_params = requires(T params) {
  { params.execution_token() } -> std::same_as<execution_token &&>;
  { params.socket() } -> std::same_as<skynet::base_socket const &>;
  { params.filter() } -> std::same_as<skynet::reactor::event_filter>;
  { params.revents() } -> std::same_as<skynet::reactor::event_filter &>;
};

}  // namespace argo::schedulers::concepts

namespace argo::schedulers {

/**
 * @brief Task scheduler implementing scheduling on a specified set of events on
 * a file descriptor
 */
template <argo::concepts::channel InChannel, argo::concepts::channel OutChannel,
          // NOLINTNEXTLINE(readability-magic-numbers)
          std::size_t ReactorCapacity = 128>
class network_scheduler : public swiss::non_copyable,
                          public swiss::non_movable {
 public:
  using in_channel_type  = InChannel;
  using out_channel_type = OutChannel;

 private:
  using in_value_type  = in_channel_type::value_type;
  using out_value_type = out_channel_type::value_type;

 private:
  static_assert(std::is_pointer_v<in_value_type>);
  static_assert(
      concepts::network_scheduler_params<std::remove_pointer_t<in_value_type>>);

 public:
  constexpr network_scheduler(InChannel & in_channel, OutChannel & out_channel)
      : reactor_(ReactorCapacity),
        in_value_(nullptr),
        in_channel_(&in_channel),
        out_channel_(&out_channel) {}

 public:
  constexpr auto in_channel() const noexcept -> in_channel_type & {
    return *in_channel_;
  }
  constexpr auto out_channel() const noexcept -> out_channel_type & {
    return *out_channel_;
  }

 public:
  /**
   * @brief Try to schedule as many as possible tasks ready to run
   */
  constexpr void run_once() {
    while (try_consume_in_value()) {
    }

    for (auto event : reactor_.poll(std::chrono::milliseconds(0))) {
      in_value_ = static_cast<in_value_type>(event.data);
      reactor_.erase(in_value_->socket());
      in_value_->revents() = event.filter;
      if (!out_channel_->try_push(in_value_->execution_token())) {
        break;
      }
      in_value_ = nullptr;
    }
  }

 private:
  skynet::reactor reactor_;
  in_value_type in_value_;
  InChannel * in_channel_;
  OutChannel * out_channel_;

 private:
  constexpr auto try_consume_in_value() -> bool {
    if (in_value_ || (!in_value_ && in_channel_->try_pop(in_value_))) {
      assert(in_value_ &&
             "network_scheduler::try_consume_in_value(): nullptr in queue");
      reactor_.insert_or_assign(in_value_->socket(), in_value_->filter(),
                                in_value_);
      in_value_ = nullptr;
      return true;
    }
    return false;
  }
};

}  // namespace argo::schedulers
