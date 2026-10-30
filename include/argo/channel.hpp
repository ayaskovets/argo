#pragma once

#include <concepts>
#include <stop_token>
#include <tuple>
#include <variant>

#include <swiss/concepts.hpp>

namespace argo::concepts {

/**
 * @brief Channel interface for a specific type
 */

template <typename T, typename Value>
concept typed_channel =
    requires(T channel, Value value, std::stop_token const & stop_token) {
      {
        channel.template try_emplace<Value>(std::move(value))
      } -> std::same_as<bool>;
      { channel.try_push(std::move(value)) } -> std::same_as<bool>;
      { channel.try_pop(value) } -> std::same_as<bool>;

      {
        channel.template emplace<Value>(std::move(value))
      } -> std::same_as<void>;
      { channel.push(std::move(value)) } -> std::same_as<void>;
      { channel.pop(value) } -> std::same_as<void>;

      {
        channel.template emplace<Value>(stop_token, std::move(value))
      } -> std::same_as<bool>;
      { channel.push(stop_token, std::move(value)) } -> std::same_as<bool>;
      { channel.pop(stop_token, value) } -> std::same_as<bool>;
    };

/**
 * @brief Channel interface for a specific type with noexcept operations
 */
template <typename T, typename Value>
concept noexcept_typed_channel =
    requires(T channel, Value value, std::stop_token const & stop_token) {
      requires noexcept(channel.template try_emplace<Value>(std::move(value)));
      requires noexcept(channel.try_push(std::move(value)));
      requires noexcept(channel.try_pop(value));

      requires noexcept(channel.template emplace<Value>(std::move(value)));
      requires noexcept(channel.push(std::move(value)));
      requires noexcept(channel.pop(value));

      requires noexcept(
          channel.template emplace<Value>(stop_token, std::move(value)));
      requires noexcept(channel.push(stop_token, std::move(value)));
      requires noexcept(channel.pop(stop_token, value));
    };

/**
 * @brief Channel handling one type of values. Expected to be thread-safe
 */
template <typename T, typename Value = typename T::value_type>
concept monochannel =
    typed_channel<T, Value> && std::is_same_v<Value, typename T::value_type>;

/**
 * @brief Channel of one type with noexcept operations
 */
template <typename T, typename Value = typename T::value_type>
concept noexcept_monochannel = noexcept_typed_channel<T, Value> &&
                               std::is_same_v<Value, typename T::value_type>;

/**
 * @brief Channel handling multiple value types. Concept does not imply that the
 * values go through a single pipe or multiple independent ones for each type.
 * Expected to be thread-safe
 * @note Differs from a monochanel in the way value type alias is defined. For
 * multichannel it is expected to have std::variant<Types...> where Types is the
 * variadic pack of all valid types the channel is able to work with
 */
template <typename T, typename... Values>
concept multichannel = requires {
  requires[]<typename... Args>(std::variant<Args...> *)->bool {
    return (sizeof...(Values) == 0 ||
            std::is_same_v<std::variant<Values...>, std::variant<Args...>>) &&
           (typed_channel<T, Args> && ...);
  }
  (static_cast<typename T::value_types *>(nullptr));
};

/**
 * @brief Multichannel with all noexcept operations
 */
template <typename T, typename... Values>
concept noexcept_multichannel = requires {
  requires[]<typename... Args>(std::variant<Args...> *)->bool {
    return (sizeof...(Values) == 0 ||
            std::is_same_v<std::variant<Values...>, std::variant<Args...>>) &&
           (noexcept_typed_channel<T, Args> && ...);
  }
  (static_cast<typename T::value_types *>(nullptr));
};

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
template <typename T, typename... Values>
concept channel_lvalue_reference =
    std::is_lvalue_reference_v<T> &&
    channel<std::remove_reference_t<T>, Values...>;

}  // namespace argo::concepts
