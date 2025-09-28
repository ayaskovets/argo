#pragma once

#include <concepts>
#include <stop_token>

#include "async/channel.hpp"

namespace core::async {

namespace concepts {

/**
 * @brief Task executor concept
 *
 * Input and output data for an executor is expected to be implemented with two
 * asynchronous channels allowing to abstract away expected thread safety for a
 * specific executor by passing the synchronization responsibility to the
 * channels
 */
template <typename T>
concept executor = requires(T executor) {
  { executor.in_channel() } -> async::concepts::channel_lvalue_reference<>;
  { executor.out_channel() } -> async::concepts::channel_lvalue_reference<>;

  requires(
      requires {
        { executor.run_once() } -> std::same_as<void>;
      } ||
      requires(std::stop_token stop_token) {
        { executor.run_detached(stop_token) } -> std::same_as<void>;
      });
};

}  // namespace concepts

}  // namespace core::async
