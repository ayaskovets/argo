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
      typename T::in_channel_type,
      std::remove_reference_t<decltype(scheduler.in_channel())>>;
  requires std::is_same_v<
      typename T::out_channel_type,
      std::remove_reference_t<decltype(scheduler.out_channel())>>;
};

}  // namespace argo::concepts
