#pragma once

#include <flat_map>
#include <type_traits>

#include <swiss/mixins.hpp>

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
  static_assert(std::is_pointer_v<typename in_channel_type::value_type>);
  using clock_t =
      std::decay_t<decltype(std::declval<typename in_channel_type::value_type>()
                                ->time_point())>::clock;
  using task_t =
      std::decay_t<decltype(std::declval<typename in_channel_type::value_type>()
                                ->task_ref())>;

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

    auto const now = clock_t::now();
    auto iter      = queue_.begin();
    try {
      for (; iter != queue_.end(); ++iter) {
        auto const & [time_point, task] = *iter;
        if (time_point > now || !out_channel_.try_push(task)) {
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

    auto const now = clock_t::now();
    auto iter      = queue_.begin();
    try {
      for (; iter < queue_.end(); ++iter) {
        auto const & [time_point, task] = *iter;
        if (time_point > now || !out_channel_.push(stop_token, task)) {
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
  typename in_channel_type::value_type in_value_;
  std::flat_map<typename clock_t::time_point, task_t> queue_;

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
