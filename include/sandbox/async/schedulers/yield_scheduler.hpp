#pragma once

#include <wip/async/channel.hpp>
#include <wip/utils/as_ref.hpp>
#include <wip/utils/mixins.hpp>

#include <concepts>
#include <deque>
#include <memory>
#include <optional>
#include <type_traits>

namespace wip::async::schedulers {

namespace concepts {

/**
 * @brief Requirements for input and output channels of yield_scheduler
 */
template <typename InChannel, typename OutChannel>
concept yield_scheduler_channels = requires(
    typename InChannel::value_type in_value) {
  requires async::concepts::channel<InChannel>;
  requires async::concepts::channel<OutChannel>;
  requires std::convertible_to<typename InChannel::value_type,
                               typename OutChannel::value_type>;

  requires std::is_nothrow_move_constructible_v<typename InChannel::value_type>;
  requires std::is_default_constructible_v<typename InChannel::value_type>;
};

}  // namespace concepts

/**
 * @brief Task scheduler implementing scheduling on a specified time point
 */
template <async::concepts::channel InChannel,
          async::concepts::channel OutChannel,
          typename Allocator = std::allocator<typename InChannel::value_type>>
class yield_scheduler : public utils::non_copyable {
 private:
  static_assert(concepts::yield_scheduler_channels<InChannel, OutChannel>);

 public:
  using allocator_value_type = typename Allocator::value_type;

 public:
  constexpr yield_scheduler(InChannel& in_channel, OutChannel& out_channel,
                            const Allocator& allocator = Allocator()) noexcept
      : in_channel_(in_channel),
        out_channel_(out_channel),
        in_value_(std::nullopt),
        queue_(allocator) {}

  constexpr yield_scheduler(yield_scheduler&& that) noexcept = default;
  constexpr yield_scheduler& operator=(yield_scheduler&& that) noexcept =
      default;
  constexpr ~yield_scheduler() noexcept = default;

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

    if (queue_.empty()) [[unlikely]] {
      return;
    }

    const auto& top = queue_.top();

    typename OutChannel::value_type out_value(top);
    // TODO: try_push throws
    if (out_channel_.try_push(std::move(out_value))) [[unlikely]] {
      queue_.pop();
    }
  }

 private:
  InChannel& in_channel_;
  OutChannel& out_channel_;

 private:
  std::optional<typename InChannel::value_type> in_value_;
  std::deque<allocator_value_type, Allocator> queue_;

 private:
  constexpr void consume_in_value() {
    queue_.emplace(utils::as_ref(*in_value_).payload());
    in_value_.reset();
  }
};

}  // namespace wip::async::schedulers
