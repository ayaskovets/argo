#pragma once

#include <concepts>
#include <type_traits>

namespace core::async {

namespace concepts {

/**
 * @brief Channel requirements for async communication between schedulers and
 * executors
 */
template <typename T, typename Value = typename T::value_t>
concept channel = requires(T channel, Value value) {
  { channel.try_push(value) } -> std::same_as<bool>;
  { channel.try_pop(value) } -> std::same_as<bool>;

  requires noexcept(channel.try_push(value));
  requires noexcept(channel.try_pop(value));

  { channel.push(value) } -> std::same_as<void>;
  { channel.pop() } -> std::convertible_to<Value>;

  requires std::is_same_v<typename T::value_t, Value>;
};

/**
 * @brief Lvalue reference to channel
 */
template <typename T,
          typename Value = typename std::remove_reference_t<T>::value_t>
concept channel_lvalue_reference = requires {
  requires std::is_lvalue_reference_v<T>;
  requires channel<std::remove_reference_t<T>, Value>;
};

}  // namespace concepts

}  // namespace core::async
