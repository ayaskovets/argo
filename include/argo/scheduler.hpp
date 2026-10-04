#pragma once

#include "channel.hpp"

namespace argo::concepts {

/**
 * @brief Task scheduler concept. The general idea of a scheduler is an event
 * loop that gets tasks to be scheduled from an input channel and sends task
 * ready to run to the output channel
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
