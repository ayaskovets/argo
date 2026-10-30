#pragma once

#include <cassert>
#include <flat_map>

#include <swiss/mixins.hpp>
#include <swiss/scope_exit.hpp>

#include "../channel.hpp"
#include "../execution_token.hpp"
#include "timer_in_value.hpp"

namespace argo::schedulers {

/**
 * @brief Task scheduler for scheduling tasks at a specified time point in
 * the future
 */
template <typename Clock,
          argo::concepts::channel<timer_in_value<Clock> *> InChannel,
          argo::concepts::channel OutChannel>
class timer_scheduler final : public swiss::non_copyable,
                              public swiss::non_movable {
 public:
  using in_channel_type  = InChannel;
  using out_channel_type = OutChannel;

 private:
  using in_value_type  = in_channel_type::value_type;
  using out_value_type = out_channel_type::value_type;

 public:
  constexpr timer_scheduler(in_channel_type & in_channel,
                            out_channel_type & out_channel) noexcept
      : queue_(),
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

    auto const now = Clock::now();
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

 private:
  std::flat_multimap<typename Clock::time_point, execution_token> queue_;
  in_value_type in_value_;
  in_channel_type * in_channel_;
  out_channel_type * out_channel_;

 private:
  constexpr auto try_consume_in_value() -> bool {
    if (in_value_ != nullptr || in_channel_->try_pop(in_value_)) {
      assert(in_value_ != nullptr &&
             "timer_scheduler::try_consume_in_value(): nullptr in queue");
      queue_.emplace(in_value_->get_time_point(),
                     in_value_->get_execution_token());
      in_value_ = nullptr;
      return true;
    }
    return false;
  }
};

}  // namespace argo::schedulers
