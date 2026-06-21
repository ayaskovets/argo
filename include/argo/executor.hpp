#pragma once

#include <type_traits>

#include "channel.hpp"

namespace argo::concepts {

/**
 * @brief Task executor concept with input/output channels
 */
template <typename T>
concept executor = requires(T executor) {
  { executor.in_channel() } -> channel_lvalue_reference<>;
  { executor.out_channel() } -> channel_lvalue_reference<>;

  requires std::is_same_v<
      typename T::in_value_type,
      typename std::decay_t<decltype(executor.in_channel())>::value_type>;
  requires std::is_same_v<
      typename T::out_value_type,
      typename std::decay_t<decltype(executor.out_channel())>::value_type>;
};

}  // namespace argo::concepts
