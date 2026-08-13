#pragma once

#include <concepts>
#include <stop_token>
#include <tuple>
#include <variant>

namespace argo::concepts {

/**
 * @brief Channel handling one type of values
 */
template <typename T, typename Value = typename T::value_type>
concept monochannel =
    requires(T channel, Value value, std::stop_token stop_token) {
      { channel.try_emplace(value) } -> std::same_as<bool>;
      { channel.try_push(value) } -> std::same_as<bool>;
      { channel.try_pop(value) } -> std::same_as<bool>;

      { channel.emplace(value) } -> std::same_as<void>;
      { channel.push(value) } -> std::same_as<void>;
      { channel.pop(value) } -> std::same_as<void>;

      { channel.emplace(stop_token, value) } -> std::same_as<bool>;
      { channel.push(stop_token, value) } -> std::same_as<bool>;
      { channel.pop(stop_token, value) } -> std::same_as<bool>;
    } && std::is_same_v<Value, typename T::value_type>;

/**
 * @brief Channel of one type with noexcept operations
 */
template <typename T, typename Value = typename T::value_type>
concept noexcept_monochannel =
    monochannel<T, Value> &&
    requires(T channel, Value value, std::stop_token stop_token) {
      requires noexcept(channel.try_emplace(value));
      requires noexcept(channel.try_push(value));
      requires noexcept(channel.try_pop(value));

      requires noexcept(channel.emplace(value));
      requires noexcept(channel.push(value));
      requires noexcept(channel.pop(value));

      requires noexcept(channel.emplace(stop_token, value));
      requires noexcept(channel.push(stop_token, value));
      requires noexcept(channel.pop(stop_token, value));
    };

/**
 * @brief Channel handling multiple value types. Concept does not imply that the
 * values go through a single pipe or multiple independent for each type or
 * anyhow
 */
template <typename T, typename... Values>
concept multichannel =
    ((sizeof...(Values) == 0 &&
          []<typename... Args>(std::variant<Args...>) -> bool {
       return (monochannel<T, Args> && ...);
     }(typename T::value_types{})) ||
     (sizeof...(Values) >= 1 && (monochannel<T, Values> && ...))) &&
    std::is_same_v<std::variant<Values...>, typename T::value_types>;

/**
 * @brief Multichannel with all noexcept operations
 */
template <typename T, typename... Values>
concept noexcept_multichannel =
    ((sizeof...(Values) == 0 &&
          []<typename... Args>(std::variant<Args...>) -> bool {
       return (noexcept_monochannel<T, Args> && ...);
     }(typename T::value_types{})) ||
     (sizeof...(Values) >= 1 && (noexcept_monochannel<T, Values> && ...))) &&
    std::is_same_v<std::variant<Values...>, typename T::value_types>;

/**
 * @brief Channel for async communication between schedulers and
 * executors
 */
template <typename T, typename... Values>
concept channel =
    (sizeof...(Values) == 0 && (monochannel<T> || multichannel<T>)) ||
    (sizeof...(Values) == 1 &&
     (monochannel<T, std::tuple_element_t<0, std::tuple<Values...>>>)) ||
    (sizeof...(Values) >= 1 && (multichannel<T, Values...>));

/**
 * @brief Channel with noexcept operations
 */
template <typename T, typename... Values>
concept noexcept_channel =
    (sizeof...(Values) == 0 &&
     (noexcept_monochannel<T> || noexcept_multichannel<T>)) ||
    (sizeof...(Values) == 1 &&
     (noexcept_monochannel<T,
                           std::tuple_element_t<0, std::tuple<Values...>>>)) ||
    (sizeof...(Values) >= 1 && (noexcept_multichannel<T, Values...>));

/**
 * @brief Lvalue reference to channel
 */
template <typename T, typename Value = std::remove_reference_t<T>::value_type>
concept channel_lvalue_reference =
    std::is_lvalue_reference_v<T> && channel<std::remove_reference_t<T>, Value>;

}  // namespace argo::concepts
