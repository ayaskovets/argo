#pragma once

#include <flat_map>

#include <swiss/concepts.hpp>
#include <swiss/mixins.hpp>
#include <swiss/scope_exit.hpp>

#include "../awaiters/timer_awaiter.hpp"
#include "../channel.hpp"

namespace argo::schedulers {

/**
 * @brief Task scheduler for scheduling tasks at a specified time point in the
 * future
 */
template <concepts::channel InChannel, concepts::channel OutChannel>
class timer_scheduler final : public swiss::non_copyable,
                              public swiss::non_movable {
 public:
  using in_channel_type  = InChannel;
  using out_channel_type = OutChannel;

 private:
  using in_value_type  = in_channel_type::value_type;
  using out_value_type = out_channel_type::value_type;
  using clock_type     = std::remove_pointer_t<in_value_type>::clock_type;
  using time_point_type =
      std::remove_pointer_t<in_value_type>::clock_type::time_point;

 private:
  static_assert(std::is_pointer_v<in_value_type>);
  static_assert(swiss::instantiation_of<std::remove_pointer_t<in_value_type>,
                                        awaiters::timer_awaiter>);

 public:
  constexpr timer_scheduler(in_channel_type & in_channel,
                            out_channel_type & out_channel) noexcept
      : in_value_(nullptr),
        queue_(),
        in_channel_(in_channel),
        out_channel_(out_channel) {}

 public:
  constexpr auto in_channel() const noexcept -> in_channel_type & {
    return in_channel_;
  }
  constexpr auto out_channel() const noexcept -> out_channel_type & {
    return out_channel_;
  }

 public:
  /**
   * @brief Try to schedule as many as possible tasks ready to run
   */
  constexpr void run_once() {
    while (try_consume_in_value()) {
    }

    auto const now = clock_type::now();
    auto iter      = queue_.begin();

    swiss::scope_exit const scope_exit(
        [&]() noexcept -> void { queue_.erase(queue_.begin(), iter); });

    for (; iter != queue_.end(); ++iter) {
      auto const & [time_point, task] = *iter;
      if (time_point > now || !out_channel_.try_push(task)) {
        break;
      }
    }
  }

  /**
   * @brief Block until at least one task gets scheduled or stop is requested
   * @note In case of an exception operation is not rolled back and already
   * scheduled tasks are going to be executed
   * @return False if no tasks were scheduled
   */
  constexpr void run_once(std::stop_token const & stop_token) {
    while (consume_in_value(stop_token)) {
    }

    auto const now = clock_type::now();
    auto iter      = queue_.begin();

    swiss::scope_exit const scope_exit(
        [&]() noexcept -> void { queue_.erase(queue_.begin(), iter); });

    for (; iter < queue_.end(); ++iter) {
      auto const & [time_point, task] = *iter;
      if (time_point > now || !out_channel_.push(stop_token, task)) {
        break;
      }
    }
  }

 private:
  in_value_type in_value_;
  std::flat_map<time_point_type, task_ref> queue_;

  // NOLINTBEGIN(cppcoreguidelines-avoid-const-or-ref-data-members)
  in_channel_type & in_channel_;
  out_channel_type & out_channel_;
  // NOLINTEND(cppcoreguidelines-avoid-const-or-ref-data-members)

 private:
  constexpr auto try_consume_in_value() -> bool {
    if ((!in_value_ && in_channel_.try_pop(in_value_)) || in_value_) {
      queue_.emplace(in_value_->time_point(), in_value_->task_ref());
      in_value_ = nullptr;
      return true;
    }
    return false;
  }

  constexpr auto consume_in_value(std::stop_token const & stop_token) -> bool {
    if ((!in_value_ && in_channel_.pop(stop_token, in_value_)) || in_value_) {
      queue_.emplace(in_value_->time_point(), in_value_->task_ref());
      in_value_ = nullptr;
      return true;
    }
    return false;
  }
};

}  // namespace argo::schedulers
