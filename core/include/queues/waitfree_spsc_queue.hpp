#pragma once

#include <atomic>
#include <memory>
#include <new>
#include <stdexcept>

#include "utils/conditionally_runtime.hpp"
#include "utils/dynamic_extent.hpp"
#include "utils/mixins.hpp"

namespace core::queues {

namespace concepts {

/**
 * @brief Default queue entry only for internal usage
 */
template <typename T, typename Value>
concept waitfree_spsc_queue_entry = requires(T entry) {
  { entry.value() } -> std::same_as<Value&>;

  requires noexcept(entry.value());

  requires std::is_nothrow_destructible_v<Value>;
  requires std::is_nothrow_move_constructible_v<Value>;
  requires std::is_nothrow_move_assignable_v<Value>;
};

template <typename T, typename Value>
concept waitfree_spsc_queue_allocator = requires {
  requires waitfree_spsc_queue_entry<typename T::value_type, Value>;
};

}  // namespace concepts

namespace detail {

/**
 * @brief Default queue entry only for internal usage
 */
template <typename Value, std::size_t Alignment = alignof(Value)>
class alignas(Alignment) waitfree_spsc_queue_entry final {
 public:
  constexpr Value& value() noexcept { return value_; }

 private:
  Value value_;
};

}  // namespace detail

/**
 * @brief Threadsafe queue for 1 producer and 1 consumer
 *
 * Implemented as a ring buffer with cached pop and push indices
 *
 * Allocated buffer always contains one additional element to allow for an
 * efficient implementation of a ring buffer indexing
 */
template <typename Value, std::unsigned_integral Index = std::size_t,
          Index Capacity = utils::kDynamicExtent<Index>,
          concepts::waitfree_spsc_queue_allocator<Value> Allocator =
              std::allocator<detail::waitfree_spsc_queue_entry<Value>>>
class waitfree_spsc_queue final : public utils::non_copyable,
                                  public utils::non_movable {
 public:
  using value_t = Value;
  using index_t = Index;
  using allocator_t = Allocator;
  using allocator_value_t = typename Allocator::value_type;

 public:
  constexpr explicit waitfree_spsc_queue(
      const allocator_t& allocator = allocator_t())
    requires(Capacity != utils::kDynamicExtent<index_t> && Capacity > 0)
      : allocator_(allocator) {
    if (!(ring_buffer_ = allocator_.allocate(*capacity_ + 1))) [[unlikely]] {
      throw std::runtime_error("failed to allocate memory");
    }
  }

  constexpr explicit waitfree_spsc_queue(
      index_t capacity, const allocator_t& allocator = allocator_t())
    requires(Capacity == utils::kDynamicExtent<index_t>)
      : allocator_(allocator), capacity_(capacity) {
    if (*capacity_ <= 0) [[unlikely]] {
      throw std::invalid_argument("capacity must be greater than zero");
    }

    if (!(ring_buffer_ = allocator_.allocate(*capacity_ + 1))) [[unlikely]] {
      throw std::runtime_error("failed to allocate memory");
    }
  }

  /**
   * @warning Destructor is only safe when called while no other thread is doing
   * operations on the queue
   */
  constexpr ~waitfree_spsc_queue() noexcept {
    clear();
    allocator_.deallocate(ring_buffer_, *capacity_ + 1);
  }

 public:
  template <typename... Args>
  constexpr bool try_emplace(Args&&... args) noexcept
    requires(std::is_nothrow_constructible_v<Value, Args...>)
  {
    const index_t push_to = push_to_.load(std::memory_order::relaxed);
    const index_t next_push_to = (push_to == *capacity_) ? 0 : push_to + 1;

    if (next_push_to == cached_pop_from_ &&
        (next_push_to ==
         (cached_pop_from_ = pop_from_.load(std::memory_order::acquire)))) {
      return false;
    }

    std::construct_at(&ring_buffer_[push_to].value(),
                      std::forward<Args>(args)...);

    push_to_.store(next_push_to, std::memory_order::release);
    return true;
  }

  constexpr bool try_push(value_t value) noexcept {
    return try_emplace(std::move(value));
  }

  template <typename... Args>
  constexpr void emplace(Args&&... args) noexcept
    requires(std::is_nothrow_constructible_v<Value, Args...>)
  {
    const index_t push_to = push_to_.load(std::memory_order::relaxed);
    const index_t next_push_to = (push_to == *capacity_) ? 0 : push_to + 1;

    while (next_push_to == cached_pop_from_ &&
           (next_push_to ==
            (cached_pop_from_ = pop_from_.load(std::memory_order::acquire))));

    std::construct_at(&ring_buffer_[push_to].value(),
                      std::forward<Args>(args)...);

    push_to_.store(next_push_to, std::memory_order::release);
  }

  constexpr void push(value_t value) noexcept { emplace(std::move(value)); }

  constexpr bool try_pop(value_t& value) noexcept {
    const index_t pop_from = pop_from_.load(std::memory_order::relaxed);
    if (pop_from == cached_push_to_ &&
        (pop_from ==
         (cached_push_to_ = push_to_.load(std::memory_order::acquire)))) {
      return false;
    }

    value = std::move(ring_buffer_[pop_from].value());
    std::destroy_at(&ring_buffer_[pop_from].value());

    const index_t next_pop_from = (pop_from == *capacity_) ? 0 : pop_from + 1;
    pop_from_.store(next_pop_from, std::memory_order::release);
    return true;
  }

  constexpr value_t pop() noexcept {
    const index_t pop_from = pop_from_.load(std::memory_order::relaxed);
    while (pop_from == cached_push_to_ &&
           (pop_from ==
            (cached_push_to_ = push_to_.load(std::memory_order::acquire))));

    value_t value(std::move(ring_buffer_[pop_from].value()));
    std::destroy_at(&ring_buffer_[pop_from].value());

    const index_t next_pop_from = (pop_from == *capacity_) ? 0 : pop_from + 1;
    pop_from_.store(next_pop_from, std::memory_order::release);
    return value;
  }

 public:
  constexpr index_t capacity() const noexcept { return *capacity_; }
  constexpr index_t size_approx() const noexcept {
    const index_t push_to = push_to_.load(std::memory_order::relaxed);
    const index_t pop_from = pop_from_.load(std::memory_order::relaxed);

    return push_to >= pop_from ? push_to - pop_from
                               : *capacity_ - pop_from - push_to + 1;
  }

 public:
  const allocator_t& get_allocator() const noexcept { return allocator_; }

 private:
  allocator_value_t* ring_buffer_ = nullptr;

  alignas(std::hardware_destructive_interference_size)
      std::atomic<index_t> push_to_ = 0;
  alignas(std::hardware_destructive_interference_size) index_t cached_push_to_ =
      0;
  alignas(std::hardware_destructive_interference_size)
      std::atomic<index_t> pop_from_ = 0;
  alignas(std::hardware_destructive_interference_size) index_t
      cached_pop_from_ = 0;

  [[no_unique_address]] allocator_t allocator_;
  [[no_unique_address]] const utils::conditionally_runtime<
      index_t, Capacity == utils::kDynamicExtent<index_t>, Capacity> capacity_;

 private:
  /**
   * @brief Preserves queue invariants. Assumes that no other thread is doing
   * any operations on the queue
   */
  constexpr void clear() noexcept {
    while (true) {
      const index_t pop_from = pop_from_.load(std::memory_order::relaxed);
      if (pop_from == cached_push_to_ &&
          (pop_from ==
           (cached_push_to_ = push_to_.load(std::memory_order::acquire)))) {
        break;
      }

      std::destroy_at(&ring_buffer_[pop_from].value());

      const index_t next_pop_from = (pop_from == *capacity_) ? 0 : pop_from + 1;
      pop_from_.store(next_pop_from, std::memory_order::release);
    }
  }
};

}  // namespace core::queues
