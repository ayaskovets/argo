#pragma once

#include <wip/async/channel.hpp>
#include <wip/async/scheduler.hpp>
#include <wip/io/poller.hpp>
#include <swiss/as_ref.hpp>
#include <swiss/mixins.hpp>

#include <unordered_map>

namespace wip::async::schedulers {

namespace concepts {

/**
 * @brief Requirements for input and output channels of io_scheduler
 */
template <typename InChannel, typename OutChannel>
concept io_scheduler_channels = requires(
    typename InChannel::value_type in_value) {
  requires async::concepts::channel<InChannel>;
  requires async::concepts::channel<OutChannel>;

  { swiss::as_ref(in_value).fd() } -> std::same_as<const io::fd&>;
  { swiss::as_ref(in_value).events() } -> std::same_as<io::poller::events_t>;
  { swiss::as_ref(in_value).payload() };

  requires std::is_nothrow_move_constructible_v<typename InChannel::value_type>;
  requires std::is_default_constructible_v<typename InChannel::value_type>;
  requires std::constructible_from<typename OutChannel::value_type,
                                   decltype(swiss::as_ref(in_value).payload())>;
};

}  // namespace concepts

namespace detail {

/**
 * @brief Default io_scheduler parameters for internal usage
 */
template <typename Payload>
class io_scheduler_input final : public async::base_scheduler_input {
 public:
  constexpr io_scheduler_input(const io::fd& fd, io::poller::events_t events,
                               Payload&& payload) noexcept
      : fd_(fd), events_(events), payload_(std::forward<Payload>(payload)) {}

 public:
  constexpr const io::fd& fd() const noexcept { return fd_; }
  constexpr io::poller::events_t events() const noexcept { return events_; }
  constexpr const Payload& payload() const noexcept { return payload_; }

 private:
  const io::fd& fd_;
  io::poller::events_t events_;
  Payload payload_;
};

}  // namespace detail

/**
 * @brief Task scheduler implementing scheduling on a specified set of events on
 * a file descriptor
 */
template <async::concepts::channel InChannel,
          async::concepts::channel OutChannel>
class io_scheduler : public swiss::non_copyable {
 private:
  static_assert(concepts::io_scheduler_channels<InChannel, OutChannel>);

 public:
  constexpr io_scheduler(InChannel& in_channel,
                         OutChannel& out_channel) noexcept
      : in_channel_(in_channel),
        out_channel_(out_channel),
        poller_([this](const io::fd& fd,
                       [[maybe_unused]] io::poller::events_t events) {
          if (events & wip::io::poller::event::kPollHup) [[unlikely]] {
            payload_by_fd_.erase(fd.native_handle());
            poller_.erase(fd);
            return;
          }

          typename OutChannel::value_type out_value(
              payload_by_fd_.at(fd.native_handle()));
          // TODO: try_push throws
          if (out_channel_.try_push(std::move(out_value))) [[unlikely]] {
            payload_by_fd_.erase(fd.native_handle());
            poller_.erase(fd);
          }
        }) {}

  constexpr io_scheduler(io_scheduler&& that) noexcept = default;
  constexpr io_scheduler& operator=(io_scheduler&& that) noexcept = default;
  constexpr ~io_scheduler() noexcept = default;

 public:
  InChannel& in_channel() noexcept { return in_channel_; }
  OutChannel& out_channel() noexcept { return out_channel_; }

 public:
  constexpr void run_once() {
    // TODO: implement bulk execution

    if (in_value_.has_value()) [[unlikely]] {
      consume_in_value();
    }

    typename InChannel::value_type in_value{};
    // TODO: try_pop throws
    if (in_channel_.try_pop(in_value)) [[likely]] {
      in_value_.emplace(std::move(in_value));
      consume_in_value();
    }

    poller_.try_poll();
  }

 private:
  InChannel& in_channel_;
  OutChannel& out_channel_;

 private:
  std::optional<typename InChannel::value_type> in_value_;
  io::poller poller_;
  std::unordered_map<int, typename OutChannel::value_type> payload_by_fd_;

 private:
  constexpr void consume_in_value() noexcept {
    poller_.insert_or_assign(swiss::as_ref(*in_value_).fd(),
                             swiss::as_ref(*in_value_).events());
    payload_by_fd_.insert_or_assign(
        swiss::as_ref(*in_value_).fd().native_handle(),
        swiss::as_ref(*in_value_).payload());
    in_value_.reset();
  }
};

}  // namespace wip::async::schedulers
