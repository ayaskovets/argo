#pragma once

#include <concepts>
#include <stop_token>

namespace argo::concepts {

/**
 * @brief Channel for async communication between schedulers and
 * executors
 */
template <typename T, typename Value = typename T::value_type>
concept channel = requires(T channel, Value value, std::stop_token stop_token) {
  { channel.try_emplace(value) } -> std::same_as<bool>;
  { channel.try_push(value) } -> std::same_as<bool>;
  { channel.try_pop(value) } -> std::same_as<bool>;

  { channel.emplace(value) } -> std::same_as<void>;
  { channel.push(value) } -> std::same_as<void>;
  { channel.pop(value) } -> std::same_as<void>;

  { channel.emplace(stop_token, value) } -> std::same_as<bool>;
  { channel.push(stop_token, value) } -> std::same_as<bool>;
  { channel.pop(stop_token, value) } -> std::same_as<bool>;

  requires std::is_same_v<typename T::value_type, Value>;
};

template <typename T, typename Value = typename T::value_type>
concept noexcept_channel =
    channel<T, Value> &&
    requires(T channel, Value value, std::stop_token stop_token) {
      requires noexcept(channel.try_emplace(value));
      requires noexcept(channel.try_push(value));
      requires noexcept(channel.try_pop(value));

      requires noexcept(channel.emplace(value));
      requires noexcept(channel.push(value));
      requires noexcept(channel.pop(value));
    };

/**
 * @brief Lvalue reference to channel
 */
template <typename T, typename Value = std::remove_reference_t<T>::value_type>
concept channel_lvalue_reference =
    std::is_lvalue_reference_v<T> && channel<std::remove_reference_t<T>, Value>;

}  // namespace argo::concepts
