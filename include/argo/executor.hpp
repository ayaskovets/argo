#pragma once

#include "channel.hpp"

namespace argo::concepts {

/**
 * @brief Task executor concept. The general idea of an executor is an event
 * loop that gets tasks to be executed from an input channel and runs them. An
 * executor should execute tasks using the execution_context interface which
 * sets up the TLS context that is used to send suspended tasks to the output
 * channel of an executor
 */
template <typename T>
concept executor = requires(T executor) {
  { executor.in_channel() } -> channel_lvalue_reference<>;
  { executor.out_channel() } -> channel_lvalue_reference<>;

  requires std::is_same_v<
      typename T::in_channel_type,
      std::remove_reference_t<decltype(executor.in_channel())>>;
  requires std::is_same_v<
      typename T::out_channel_type,
      std::remove_reference_t<decltype(executor.out_channel())>>;
};

}  // namespace argo::concepts
