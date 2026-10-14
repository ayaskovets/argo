#pragma once

#include <flat_map>
#include <variant>

#include <swiss/mixins.hpp>
#include <swiss/scope_exit.hpp>

#include "../channel.hpp"
#include "../execution_token.hpp"

namespace argo::schedulers::concepts {

template <typename T, typename Clock>
concept timer_scheduler_params = requires(T params) {
  { params.execution_token() } -> std::same_as<execution_token &>;
  { params.time_point() } -> std::same_as<typename Clock::time_point &>;
};

}  // namespace argo::schedulers::concepts

namespace argo::schedulers {

/**
 * @brief Task scheduler for scheduling tasks at a specified time point in
 * the future
 */
template <typename Clock, argo::concepts::channel InChannel,
          argo::concepts::channel OutChannel>
class timer_scheduler final : public swiss::non_copyable,
                              public swiss::non_movable {
 public:
  using in_channel_type  = InChannel;
  using out_channel_type = OutChannel;
  using clock_type       = Clock;
  using time_point_type  = clock_type::time_point;

 private:
  using in_value_type  = in_channel_type::value_type;
  using out_value_type = out_channel_type::value_type;

 private:
  static_assert(concepts::timer_scheduler_params<in_value_type, Clock>);

 public:
  constexpr timer_scheduler(in_channel_type & in_channel,
                            out_channel_type & out_channel) noexcept
      : in_value_(std::monostate{}),
        in_channel_(&in_channel),
        out_channel_(&out_channel) {}

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
      auto [time_point, task] = *iter;
      if (time_point > now || !out_channel_->try_push(std::move(task))) {
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
      auto [time_point, task] = *iter;
      if (time_point > now ||
          !out_channel_->push(stop_token, std::move(task))) {
        break;
      }
    }
  }

 private:
  std::flat_map<time_point_type, execution_token> queue_;
  std::variant<std::monostate, in_value_type> in_value_;
  in_channel_type * in_channel_;
  out_channel_type * out_channel_;

 private:
  constexpr auto try_consume_in_value() -> bool {
    if (in_value_.index() != 0 ||
        (in_value_.index() == 0 && in_channel_->try_pop(in_value_))) {
      queue_.emplace(std::get<1>(in_value_).time_point(),
                     std::move(std::get<1>(in_value_).execution_token()));
      in_value_ = std::monostate{};
      return true;
    }
    return false;
  }

  constexpr auto consume_in_value(std::stop_token const & stop_token) -> bool {
    if (in_value_.index() != 0 ||
        (in_value_.index() == 0 && in_channel_->pop(stop_token, in_value_))) {
      queue_.emplace(std::get<1>(in_value_).time_point(),
                     std::move(std::get<1>(in_value_).execution_token()));
      in_value_ = std::monostate{};
      return true;
    }
    return false;
  }
};

}  // namespace argo::schedulers
