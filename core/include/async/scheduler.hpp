#pragma once

#include <concepts>
#include <stop_token>

#include "async/channel.hpp"

namespace core::async {

/**
 * @brief Common type for all scheduler input parameters to use with any
 * multiplexed channel that supports dynamic dispatching of heterogeneous
 * awaiter parameters into the correct scheduler
 */
class base_scheduler_input {
 public:
  constexpr virtual ~base_scheduler_input() noexcept = default;
};

namespace concepts {

/**
 * @brief Task scheduler concept
 *
 * Input and output data for a scheduler is expected to be implemented with two
 * asynchronous channels allowing to abstract away expected thread safety for a
 * specific scheduler by passing the synchronization responsibility to the
 * channels
 *
 * A scheduler represents an event loop with some specific logic related to when
 * to "schedule" (aka send to the out channel) the data from input channel
 */
template <typename T>
concept scheduler = requires(T scheduler) {
  { scheduler.in_channel() } -> async::concepts::channel_lvalue_reference<>;
  { scheduler.out_channel() } -> async::concepts::channel_lvalue_reference<>;

  requires(
      requires {
        { scheduler.run_once() } -> std::same_as<void>;
      } ||
      requires(std::stop_token stop_token) {
        { scheduler.run_detached(stop_token) } -> std::same_as<void>;
      });
};

}  // namespace concepts

}  // namespace core::async
