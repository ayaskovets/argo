#pragma once

#include <memory>
#include <stdexcept>

#include "utils/conditionally_runtime.hpp"
#include "utils/dynamic_extent.hpp"
#include "utils/mixins.hpp"

namespace core::queues {

namespace concepts {

/**
 * @brief Queue entry requirements
 */
template <typename T, typename Value>
concept single_threaded_queue_entry = requires(T entry) {
  { entry.value() } -> std::same_as<Value&>;

  requires noexcept(entry.value());

  requires std::is_nothrow_destructible_v<Value>;
  requires std::is_nothrow_move_constructible_v<Value>;
  requires std::is_nothrow_move_assignable_v<Value>;
};

template <typename T, typename Value>
concept single_threaded_queue_allocator = requires {
  requires single_threaded_queue_entry<typename T::value_type, Value>;
};

}  // namespace concepts

namespace detail {

/**
 * @brief Default queue entry only for internal usage
 */
template <typename Value, std::size_t Alignment = alignof(Value)>
class alignas(Alignment) single_threaded_queue_entry final {
 public:
  constexpr Value& value() noexcept { return value_; }

 private:
  Value value_;
};

}  // namespace detail

/**
 * @brief Ring buffer for a single-threaded usage. Not threadsafe
 *
 * Implemented as a ring buffer
 */
template <typename Value, std::unsigned_integral Index = std::size_t,
          Index Capacity = utils::kDynamicExtent<Index>,
          concepts::single_threaded_queue_allocator<Value> Allocator =
              std::allocator<detail::single_threaded_queue_entry<Value>>>
class single_threaded_queue final : utils::non_copyable, utils::non_movable {
 public:
  using value_t = Value;
  using index_t = Index;
  using allocator_t = Allocator;
  using allocator_value_t = typename Allocator::value_type;

 public:
  constexpr explicit single_threaded_queue(
      const allocator_t& allocator = allocator_t())
    requires(Capacity != utils::kDynamicExtent<index_t> && Capacity > 0)
      : allocator_(allocator) {
    if (!(ring_buffer_ = allocator_.allocate(*capacity_))) [[unlikely]] {
      throw std::runtime_error("failed to allocate memory");
    }
  }

  constexpr explicit single_threaded_queue(
      index_t capacity, const allocator_t& allocator = allocator_t())
    requires(Capacity == utils::kDynamicExtent<index_t>)
      : allocator_(allocator), capacity_(capacity) {
    if (*capacity_ <= 0) [[unlikely]] {
      throw std::invalid_argument("capacity must be greater than zero");
    }

    if (!(ring_buffer_ = allocator_.allocate(*capacity_))) [[unlikely]] {
      throw std::runtime_error("failed to allocate memory");
    }
  }

  /**
   * @warning Destructor is only safe when called while no other thread is doing
   * operations on the queue
   */
  constexpr ~single_threaded_queue() noexcept {
    clear();
    allocator_.deallocate(ring_buffer_, *capacity_);
  }

 public:
  template <typename... Args>
  constexpr bool try_emplace(Args&&... args) noexcept
    requires(std::is_nothrow_constructible_v<Value, Args...>)
  {
    if (size_ == *capacity_) {
      return false;
    }

    std::construct_at(&ring_buffer_[push_to_].value(),
                      std::forward<Args>(args)...);

    const index_t next_push_to =
        (push_to_ == *capacity_ - 1) ? 0 : push_to_ + 1;
    push_to_ = next_push_to;
    size_ += 1;
    return true;
  }

  constexpr bool try_push(value_t value) noexcept {
    return try_emplace(std::move(value));
  }

  template <typename... Args>
  constexpr void emplace(Args&&... args)
    requires(std::is_nothrow_constructible_v<Value, Args...>)
  {
    if (!try_emplace(std::forward<Args>(args)...)) {
      throw std::runtime_error("queue is full");
    }
  }

  constexpr void push(value_t value) { emplace(std::move(value)); }

  constexpr bool try_pop(value_t& value) noexcept {
    if (size_ == 0) {
      return false;
    }

    value = std::move(ring_buffer_[pop_from_].value());
    std::destroy_at(&ring_buffer_[pop_from_].value());

    const index_t next_pop_from =
        (pop_from_ == *capacity_ - 1) ? 0 : pop_from_ + 1;
    pop_from_ = next_pop_from;
    size_ -= 1;
    return true;
  }

  constexpr value_t pop() {
    if (size_ == 0) {
      throw std::runtime_error("queue is empty");
    }

    value_t value(std::move(ring_buffer_[pop_from_].value()));
    std::destroy_at(&ring_buffer_[pop_from_].value());

    const index_t next_pop_from =
        (pop_from_ == *capacity_ - 1) ? 0 : pop_from_ + 1;
    pop_from_ = next_pop_from;
    size_ -= 1;
    return value;
  }

 public:
  constexpr bool empty() const noexcept { return size_ == 0; }
  constexpr index_t size() const noexcept { return size_; }
  constexpr index_t capacity() const noexcept { return *capacity_; }

 public:
  const allocator_t& get_allocator() const noexcept { return allocator_; }

 private:
  allocator_value_t* ring_buffer_ = nullptr;

  index_t push_to_ = 0;
  index_t pop_from_ = 0;
  index_t size_ = 0;

  [[no_unique_address]] allocator_t allocator_;
  [[no_unique_address]] const utils::conditionally_runtime<
      index_t, Capacity == utils::kDynamicExtent<index_t>, Capacity> capacity_;

 private:
  /**
   * @brief Preserves queue invariants
   */
  constexpr void clear() noexcept {
    while (true) {
      if (size_ == 0) {
        break;
      }

      std::destroy_at(&ring_buffer_[pop_from_].value());

      const index_t next_pop_from =
          (pop_from_ == *capacity_ - 1) ? 0 : pop_from_ + 1;
      pop_from_ = next_pop_from;
      size_ -= 1;
    }
  }
};

}  // namespace core::queues
