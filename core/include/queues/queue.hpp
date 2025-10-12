#pragma once

#include <concepts>

namespace core::queues {

namespace concepts {

/**
 * @brief Channel requirements for async communication between schedulers and
 * executors
 */
template <typename T>
concept queue = requires(T queue, typename T::value_t value) {
  { queue.try_emplace(value) } -> std::same_as<bool>;
  { queue.emplace(value) } -> std::same_as<void>;

  { queue.try_push(value) } -> std::same_as<bool>;
  { queue.push(value) } -> std::same_as<void>;

  { queue.try_pop(value) } -> std::same_as<bool>;
  { queue.pop() } -> std::convertible_to<typename T::value_t>;

  { queue.capacity() } -> std::convertible_to<typename T::index_t>;
  { queue.size() } -> std::same_as<typename T::index_t>;

  { queue.get_allocator() } -> std::same_as<typename T::allocator_t>;
};

}  // namespace concepts

}  // namespace core::queues
