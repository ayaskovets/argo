#pragma once

#include <concepts>
#include <stop_token>

#include "async/channel.hpp"

namespace core::async {

class base_scheduler_parameters {
 public:
  constexpr virtual ~base_scheduler_parameters() noexcept {}
};

namespace concepts {

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

  requires std::is_base_of_v<async::base_scheduler_parameters,
                             typename T::params_t>;
};

}  // namespace concepts

}  // namespace core::async
