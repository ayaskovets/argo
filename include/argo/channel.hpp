#pragma once

#include <concepts>

namespace argo::concepts {

/**
 * @brief Channel for async communication between schedulers and
 * executors
 */
template <typename T, typename Value = typename T::value_type>
concept channel = requires(T channel, Value value) {
  { channel.try_emplace(value) } -> std::same_as<bool>;
  { channel.try_push(value) } -> std::same_as<bool>;
  { channel.try_pop(value) } -> std::same_as<bool>;

  requires noexcept(channel.try_emplace(value));
  requires noexcept(channel.try_push(value));
  requires noexcept(channel.try_pop(value));

  requires std::is_same_v<typename T::value_type, Value>;
};

/**
 * @brief Lvalue reference to channel
 */
template <typename T,
          typename Value = typename std::remove_reference_t<T>::value_type>
concept channel_lvalue_reference =
    std::is_lvalue_reference_v<T> && channel<std::remove_reference_t<T>, Value>;

}  // namespace argo::concepts
