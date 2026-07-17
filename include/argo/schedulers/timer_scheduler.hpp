#pragma once

#include <flat_map>

#include <swiss/mixins.hpp>

#include "../channel.hpp"

namespace argo::schedulers {

/**
 * @brief Task scheduler for scheduling tasks at a specified time point in the
 * future
 * @note Input channel value is expected to be a pointer to an aggregate of two
 * values (time point, task handle)
 */
template <concepts::channel InChannel, concepts::channel OutChannel>
class timer_scheduler final : public swiss::non_copyable,
                              public swiss::non_movable {
 private:
  /**
   * @note Input channel values are expected to be pointers to scheduling
   * data that can possibly outlive the channel. Scheduler does not own the
   * scheduling data
   */
  static_assert(std::is_pointer_v<typename InChannel::value_type>);

  using time_point_type = std::decay_t<decltype(std::get<0>(
      *std::declval<typename InChannel::value_type>()))>;
  using handle_type     = std::decay_t<decltype(std::get<1>(
      *std::declval<typename InChannel::value_type>()))>;

 public:
  using in_channel_type  = InChannel;
  using out_channel_type = OutChannel;

 public:
  constexpr timer_scheduler(in_channel_type & in_channel,
                            out_channel_type & out_channel) noexcept
      : in_channel_(in_channel), out_channel_(out_channel) {}

 public:
  constexpr auto in_channel() const noexcept -> in_channel_type & {
    return in_channel_;
  }
  constexpr auto out_channel() const noexcept -> out_channel_type & {
    return out_channel_;
  }

 public:
  /**
   * @brief Try to schedule at least one task ready to run
   */
  constexpr void run_once() {
    while (try_consume_in_value()) {
    }

    auto const now = time_point_type::clock::now();
    auto iter      = queue_.begin();
    try {
      for (; iter != queue_.end(); ++iter) {
        auto const & [time_point, handle] = *iter;
        if (time_point > now || !out_channel_.try_push(handle)) {
          break;
        }
      }

      queue_.erase(queue_.begin(), iter);
    } catch (std::exception const &) {
      queue_.erase(queue_.begin(), iter);
      throw;
    }
  }

  /**
   * @brief Block until at least one task gets scheduled or stop is requested
   * @note In case of an exception operation is not rolled back and already
   * scheduled tasks are going to be executed
   * @return False if no tasks were scheduled
   */
  constexpr auto run_once(std::stop_token const & stop_token) -> bool {
    while (consume_in_value(stop_token)) {
    }

    auto const now = time_point_type::clock::now();
    auto iter      = queue_.begin();
    try {
      for (; iter < queue_.end(); ++iter) {
        auto const & [time_point, handle] = *iter;
        if (time_point > now || !out_channel_.push(stop_token, handle)) {
          break;
        }
      }

      queue_.erase(queue_.begin(), iter);
    } catch (std::exception const &) {
      queue_.erase(queue_.begin(), iter);
      throw;
    }

    return std::distance(queue_.begin(), iter) != 0;
  }

 private:
  // NOLINTBEGIN(cppcoreguidelines-avoid-const-or-ref-data-members)
  in_channel_type & in_channel_;
  out_channel_type & out_channel_;
  // NOLINTEND(cppcoreguidelines-avoid-const-or-ref-data-members)

 private:
  typename in_channel_type::value_type in_value_ = nullptr;
  std::flat_map<time_point_type, handle_type> queue_;

 private:
  constexpr auto try_consume_in_value() -> bool {
    if ((!in_value_ && in_channel_.try_pop(in_value_)) || in_value_) {
      auto const & [time_point, handle] = *in_value_;
      queue_.emplace(time_point, handle);
      in_value_ = nullptr;
      return true;
    }
    return false;
  }

  constexpr auto consume_in_value(std::stop_token const & stop_token) -> bool {
    if ((!in_value_ && in_channel_.pop(stop_token, in_value_)) || in_value_) {
      auto const & [time_point, handle] = *in_value_;
      queue_.emplace(time_point, handle);
      in_value_ = nullptr;
      return true;
    }
    return false;
  }
};

}  // namespace argo::schedulers
