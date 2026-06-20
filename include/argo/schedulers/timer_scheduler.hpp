#pragma once

#include <coroutine>
#include <optional>
#include <queue>

#include <swiss/mixins.hpp>

#include "../channel.hpp"

namespace argo::schedulers {

/**
 * @brief Task scheduler for scheduling tasks to run after some specified time
 * point
 */
template <typename Clock,
          concepts::channel<
              std::pair<typename Clock::time_point, std::coroutine_handle<>>>
              InChannel,
          concepts::channel<std::coroutine_handle<>> OutChannel,
          typename Allocator = std::allocator<typename InChannel::value_type>>
class timer_scheduler final : public swiss::non_copyable,
                              public swiss::non_movable {
 public:
  using value_type =
      std::pair<typename Clock::time_point, std::coroutine_handle<>>;

 public:
  constexpr timer_scheduler(InChannel & in_channel, OutChannel & out_channel,
                            Allocator const & allocator = Allocator()) noexcept
      : in_channel_(in_channel), out_channel_(out_channel), queue_(allocator) {}

 public:
  constexpr auto in_channel() const noexcept -> InChannel & {
    return in_channel_;
  }
  constexpr auto out_channel() const noexcept -> OutChannel & {
    return out_channel_;
  }

 public:
  constexpr void run_once() noexcept {
    if (!in_value_.has_value()) {
      /**
       * @note Popping a non-optional value here is required because
       * std::optional::operator= is not marked noexcept for nothrow copyable
       * types unfortunately
       */

      if (typename InChannel::value_type in_value{};
          in_channel_.try_pop(in_value)) {
        in_value_.emplace(std::move(in_value));
      }
    }

    if (in_value_.has_value()) {
      try {
        queue_.emplace(std::move(*in_value_));
        in_value_.reset();
        // NOLINTNEXTLINE(bugprone-empty-catch)
      } catch (std::exception const &) {
      }
    }

    if (queue_.empty()) {
      return;
    }

    auto const now                      = Clock::now();
    auto const & [schedule_at, payload] = queue_.top();
    if (schedule_at > now) {
      return;
    }

    typename OutChannel::value_type out_value(payload);
    if (out_channel_.try_emplace(std::move(out_value))) [[likely]] {
      queue_.pop();
    }
  }

 private:
  // NOLINTBEGIN(cppcoreguidelines-avoid-const-or-ref-data-members)
  InChannel & in_channel_;
  OutChannel & out_channel_;
  // NOLINTEND(cppcoreguidelines-avoid-const-or-ref-data-members)

 private:
  std::optional<typename InChannel::value_type> in_value_;
  std::priority_queue<typename InChannel::value_type,
                      std::deque<typename InChannel::value_type, Allocator>,
                      std::greater<typename InChannel::value_type>>
      queue_;
};

}  // namespace argo::schedulers
