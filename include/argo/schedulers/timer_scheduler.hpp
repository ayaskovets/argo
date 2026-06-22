#pragma once

#include <optional>
#include <queue>

#include <swiss/mixins.hpp>

#include "../channel.hpp"

namespace argo::schedulers {

namespace detail::concepts {

template <typename T, typename Value = T::value_type>
concept timer_scheduler_in_channel =
    ::argo::concepts::channel<T> && std::is_pointer_v<Value> &&
    requires(Value value,
             std::remove_pointer_t<Value>::clock_type::time_point time_point,
             std::remove_pointer_t<Value>::task_type task) {
      { value->schedule_at() } -> std::convertible_to<decltype(time_point)>;
      { value->schedule_task() } -> std::convertible_to<decltype(task)>;
    };

template <typename T, typename InChannel, typename Value = T::value_type>
concept timer_scheduler_out_channel =
    timer_scheduler_in_channel<InChannel> &&
    std::is_same_v<Value, typename std::remove_pointer_t<
                              typename InChannel::value_type>::task_type>;

}  // namespace detail::concepts

/**
 * @brief Task scheduler for scheduling tasks to run after some specified time
 * point
 * @note Input channel is expected to pass pointers to the awaiter
 * implementation
 */
template <detail::concepts::timer_scheduler_in_channel InChannel,
          detail::concepts::timer_scheduler_out_channel<InChannel> OutChannel>
class timer_scheduler final : public swiss::non_copyable,
                              public swiss::non_movable {
 public:
  using in_value_type  = typename InChannel::value_type;
  using out_value_type = typename OutChannel::value_type;

 public:
  constexpr timer_scheduler(InChannel & in_channel,
                            OutChannel & out_channel) noexcept
      : in_channel_(in_channel), out_channel_(out_channel) {}

 public:
  constexpr auto in_channel() const noexcept -> InChannel & {
    return in_channel_;
  }
  constexpr auto out_channel() const noexcept -> OutChannel & {
    return out_channel_;
  }

 public:
  /**
   * @brief Try to schedule at least one ready task
   */
  constexpr void run_once() {
    // TODO: impl
  }

  /**
   * @brief Block until at least one task gets scheduled
   * @return False if no tasks were scheduled
   */
  constexpr auto run_once(std::stop_token const &) -> bool {
    // TODO: impl
    return false;
  }

 private:
  // NOLINTBEGIN(cppcoreguidelines-avoid-const-or-ref-data-members)
  InChannel & in_channel_;
  OutChannel & out_channel_;
  // NOLINTEND(cppcoreguidelines-avoid-const-or-ref-data-members)

 private:
  using clock_t              = std::remove_pointer_t<in_value_type>::clock_type;
  using time_point_t         = clock_t::time_point;
  using task_t               = std::remove_pointer_t<in_value_type>::task_type;
  using intermediate_value_t = std::pair<time_point_t, task_t>;

  std::optional<intermediate_value_t> in_value_;
  std::priority_queue<
      intermediate_value_t,
      std::deque<intermediate_value_t, std::allocator<intermediate_value_t>>,
      std::greater<intermediate_value_t>>
      queue_;

 private:
  constexpr void maybe_consume_in_value()
    requires(std::is_nothrow_constructible_v<intermediate_value_t,
                                             time_point_t &&, task_t &&>)
  {
    if (!in_value_.has_value()) {
      if (in_value_type in_value{}; in_channel_.try_pop(in_value)) {
        /**
         * @note This method is exception safe only if this construction is
         * noexcept so an element from the input queue cannot be lost
         */
        in_value_.emplace(std::move(in_value->schedule_at()),
                          std::move(in_value->schedule_task()));
      }
    }

    if (in_value_.has_value()) {
      queue_.emplace(std::move(*in_value_));
      in_value_.reset();
    }
  }
};

}  // namespace argo::schedulers
