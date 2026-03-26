#pragma once

#include <concepts>

namespace wip::queues {

namespace concepts {

namespace detail {

/**
 * @brief Channel requirements for async communication between schedulers and
 * executors
 */
template <typename T, bool Noexcept = false>
concept queue = requires(T queue, typename T::value_type value) {
  { queue.try_emplace(value) } -> std::same_as<bool>;
  requires !Noexcept || noexcept(queue.try_emplace(value));

  { queue.emplace(value) } -> std::same_as<void>;
  requires !Noexcept || noexcept(queue.emplace(value));

  { queue.try_push(value) } -> std::same_as<bool>;
  requires !Noexcept || noexcept(queue.try_push(value));

  { queue.push(value) } -> std::same_as<void>;
  requires !Noexcept || noexcept(queue.push(value));

  { queue.try_pop(value) } -> std::same_as<bool>;
  requires !Noexcept || noexcept(queue.try_pop(value));

  { queue.pop() } -> std::convertible_to<typename T::value_type>;
  requires !Noexcept || noexcept(queue.pop());

  { queue.capacity() } -> std::convertible_to<typename T::index_type>;
  requires noexcept(queue.capacity());

  { queue.size() } -> std::same_as<typename T::index_type>;
  requires noexcept(queue.size());

  { queue.get_allocator() } -> std::same_as<typename T::allocator_type>;
  requires noexcept(queue.get_allocator());
};

}  // namespace detail

template <typename T>
concept queue = detail::queue<T, false>;

template <typename T>
concept noexcept_queue = detail::queue<T, true>;

}  // namespace concepts

}  // namespace wip::queues
