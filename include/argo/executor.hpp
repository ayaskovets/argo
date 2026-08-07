#pragma once

#include <type_traits>

#include "channel.hpp"

namespace argo {

class execution_context;

}  // namespace argo

namespace argo::concepts {

/**
 * @brief Task executor concept with input/output channels
 */
template <typename T>
concept executor = requires(T executor) {
  { executor.in_channel() } -> channel_lvalue_reference<>;
  { executor.out_channel() } -> channel_lvalue_reference<>;
  { executor.context() } -> std::same_as<execution_context &>;

  requires std::is_same_v<
      typename T::in_channel_type,
      std::remove_reference_t<decltype(executor.in_channel())>>;
  requires std::is_same_v<
      typename T::out_channel_type,
      std::remove_reference_t<decltype(executor.out_channel())>>;
};

}  // namespace argo::concepts
