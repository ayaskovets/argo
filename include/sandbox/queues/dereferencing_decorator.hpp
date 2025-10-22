#pragma once

#include <sandbox/queues/queue.hpp>

#include <utility>

namespace sandbox::queues {

template <concepts::queue Queue>
class dereferencing_decorator : public Queue {
 public:
  using value_t = decltype(*std::declval<typename Queue::value_t>());

 public:
  using Queue::Queue;

 public:
  template <typename... Args>
  constexpr void emplace(Args&&...) = delete;

  template <typename... Args>
  constexpr bool try_emplace(Args&&...) = delete;

  constexpr bool try_push(value_t& value) noexcept(
      noexcept(Queue::try_push(&value))) {
    return Queue::try_push(&value);
  }

  constexpr void push(value_t& value) noexcept(noexcept(Queue::push(&value))) {
    Queue::push(&value);
  }

  constexpr value_t pop() noexcept(noexcept(Queue::pop())) {
    return *Queue::pop();
  }

  constexpr bool try_pop(value_t& value) noexcept(
      noexcept(Queue::try_pop(std::declval<typename Queue::value_t&>()))) {
    typename Queue::value_t pointer = nullptr;
    if (Queue::try_pop(pointer)) {
      value = std::move(*pointer);
      return true;
    } else {
      return false;
    }
  }
};

}  // namespace sandbox::queues