#pragma once

#include <concepts>
#include <optional>
#include <queue>

#include "async/channel.hpp"
#include "async/scheduler.hpp"
#include "utils/as_ref.hpp"
#include "utils/mixins.hpp"

namespace core::async::schedulers {

namespace concepts {

/**
 * @brief Requirements for input and output channels of timer_scheduler
 */
template <typename Clock, typename InChannel, typename OutChannel>
concept timer_scheduler_channels = requires(
    typename InChannel::value_t in_value) {
  requires async::concepts::channel<InChannel>;
  requires async::concepts::channel<OutChannel>;

  {
    utils::as_ref(in_value).schedule_at()
  } -> std::convertible_to<typename Clock::time_point>;
  { utils::as_ref(in_value).payload() };

  requires std::is_nothrow_move_constructible_v<typename InChannel::value_t>;
  requires std::is_default_constructible_v<typename InChannel::value_t>;
  requires std::constructible_from<typename OutChannel::value_t,
                                   decltype(utils::as_ref(in_value).payload())>;
};

}  // namespace concepts

namespace detail {

/**
 * @brief Default timer_scheduler parameters for internal usage
 */
template <typename Clock, typename Payload>
class timer_scheduler_input final : public async::base_scheduler_input {
 public:
  constexpr timer_scheduler_input() noexcept = default;
  constexpr timer_scheduler_input(typename Clock::time_point&& schedule_at,
                                  Payload&& payload) noexcept
      : schedule_at_(std::move(schedule_at)), payload_(std::move(payload)) {}

 public:
  constexpr const Payload& payload() const noexcept { return payload_; }
  constexpr const typename Clock::time_point& schedule_at() const noexcept {
    return schedule_at_;
  }

 private:
  typename Clock::time_point schedule_at_;
  Payload payload_;
};

}  // namespace detail

/**
 * @brief Task scheduler implementing scheduling on a specified time point
 */
template <typename Clock, async::concepts::channel InChannel,
          async::concepts::channel OutChannel,
          typename Allocator = std::allocator<std::pair<
              typename Clock::time_point, typename OutChannel::value_t>>>
class timer_scheduler : public utils::non_copyable {
 private:
  static_assert(
      concepts::timer_scheduler_channels<Clock, InChannel, OutChannel>);

 public:
  using allocator_value_t = typename Allocator::value_type;

 public:
  constexpr timer_scheduler(InChannel& in_channel, OutChannel& out_channel,
                            const Allocator& allocator = Allocator()) noexcept
      : in_channel_(in_channel),
        out_channel_(out_channel),
        in_value_(std::nullopt),
        queue_(allocator) {}

  constexpr timer_scheduler(timer_scheduler&& that) noexcept = default;
  constexpr timer_scheduler& operator=(timer_scheduler&& that) noexcept =
      default;
  constexpr ~timer_scheduler() noexcept = default;

 public:
  InChannel& in_channel() noexcept { return in_channel_; }
  OutChannel& out_channel() noexcept { return out_channel_; }

 public:
  constexpr void run_once() {
    // TODO: implement bulk execution

    if (in_value_.has_value()) [[unlikely]] {
      consume_in_value();
    }

    typename InChannel::value_t in_value{};
    // TODO: try_pop throws
    if (in_channel_.try_pop(in_value)) [[likely]] {
      in_value_.emplace(std::move(in_value));
      consume_in_value();
    }

    if (queue_.empty()) [[unlikely]] {
      return;
    }

    const auto now = Clock::now();
    const auto& [schedule_at, payload] = queue_.top();
    if (schedule_at > now) {
      return;
    }

    typename OutChannel::value_t out_value(payload);
    // TODO: try_push throws
    if (out_channel_.try_push(std::move(out_value))) [[unlikely]] {
      queue_.pop();
    }
  }

 private:
  InChannel& in_channel_;
  OutChannel& out_channel_;

 private:
  std::optional<typename InChannel::value_t> in_value_;
  std::priority_queue<allocator_value_t,
                      std::vector<allocator_value_t, Allocator>,
                      std::greater<allocator_value_t>>
      queue_;

 private:
  constexpr void consume_in_value() {
    queue_.emplace(utils::as_ref(*in_value_).schedule_at(),
                   utils::as_ref(*in_value_).payload());
    in_value_.reset();
  }
};

}  // namespace core::async::schedulers
