#pragma once

#include "channel.hpp"

namespace argo::concepts {

/**
 * @brief Task scheduler with input/output channels
 */
template <typename T>
concept scheduler = requires(T scheduler) {
  { scheduler.in_channel() } -> concepts::channel_lvalue_reference<>;
  { scheduler.out_channel() } -> concepts::channel_lvalue_reference<>;

  requires std::is_same_v<
      typename T::in_value_type,
      typename std::decay_t<decltype(scheduler.in_channel())>::value_type>;
  requires std::is_same_v<
      typename T::out_value_type,
      typename std::decay_t<decltype(scheduler.out_channel())>::value_type>;
};

}  // namespace argo::concepts
