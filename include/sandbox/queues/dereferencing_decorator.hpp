#pragma once

#include <sandbox/queues/queue.hpp>

#include <utility>

namespace sandbox::queues {

// TODO: remove this
template <concepts::queue Queue>
class dereferencing_decorator : public Queue {
 public:
  using value_type = decltype(*std::declval<typename Queue::value_type>());

 public:
  using Queue::Queue;

 public:
  template <typename... Args>
  constexpr void emplace(Args&&...) = delete;

  template <typename... Args>
  constexpr bool try_emplace(Args&&...) = delete;

  constexpr bool try_push(value_type& value) noexcept(
      noexcept(Queue::try_push(&value))) {
    return Queue::try_push(&value);
  }

  constexpr void push(value_type& value) noexcept(
      noexcept(Queue::push(&value))) {
    Queue::push(&value);
  }

  constexpr value_type pop() noexcept(noexcept(Queue::pop())) {
    return *Queue::pop();
  }

  constexpr bool try_pop(value_type& value) noexcept(
      noexcept(Queue::try_pop(std::declval<typename Queue::value_type&>()))) {
    typename Queue::value_type pointer = nullptr;
    if (Queue::try_pop(pointer)) {
      value = std::move(*pointer);
      return true;
    } else {
      return false;
    }
  }
};

}  // namespace sandbox::queues