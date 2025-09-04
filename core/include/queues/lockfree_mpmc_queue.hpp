#pragma once

#include <atomic>
#include <memory>
#include <stdexcept>

#include "utils/conditionally_runtime.hpp"
#include "utils/constants.hpp"
#include "utils/mixins.hpp"
#include "utils/predicates.hpp"

namespace core::queues {

/**
 * @brief Queue entry requirements
 *
 * Requires seqnum() to avoid the ABA problem
 */
template <typename T, typename Value, typename Index>
concept lockfree_mpmc_queue_entry = requires(T entry) {
  requires std::atomic<T>::is_always_lock_free;
  { entry.value() } -> std::same_as<Value&>;
  { entry.seqnum() } -> std::same_as<Index&>;

  requires noexcept(entry.value());
  requires noexcept(entry.seqnum());

  requires std::atomic<T>::is_always_lock_free;

  requires std::is_nothrow_default_constructible_v<T>;
  requires std::is_trivially_destructible_v<T>;
  requires std::is_nothrow_destructible_v<T>;
  requires std::is_trivially_copy_constructible_v<T>;
  requires std::is_nothrow_copy_constructible_v<T>;
  requires std::is_trivially_move_assignable_v<T>;
  requires std::is_nothrow_move_assignable_v<T>;
} && std::unsigned_integral<Index>;

namespace detail {

/**
 * @brief Default queue entry only for internal usage
 */
template <typename Value, std::unsigned_integral Index = std::size_t,
          std::size_t Alignment = alignof(std::pair<Value, Index>)>
class alignas(Alignment) lockfree_mpmc_queue_entry_impl final {
 public:
  constexpr Value& value() noexcept { return value_; }
  constexpr Index& seqnum() noexcept { return seqnum_; }

 private:
  Value value_;
  Index seqnum_;
};

}  // namespace detail

/**
 * @brief Threadsafe queue for many producers and many consumers
 *
 * Implemented with 128-bit CAS. The last bit of seqnum indicates whether the
 * element is occupied
 *
 * Allocator value_type must be std::atomic
 */
template <typename Value, std::unsigned_integral Index = std::size_t,
          Index Capacity = utils::kRuntimeDefinedCapacity<Index>,
          typename Allocator = std::allocator<std::atomic<
              detail::lockfree_mpmc_queue_entry_impl<Value, Index>>>>
class lockfree_mpmc_queue final : utils::non_copyable, utils::non_movable {
 private:
  using atomic_entry_t = typename Allocator::value_type;

 public:
  using value_t = Value;
  using index_t = Index;
  using allocator_t = Allocator;
  using entry_t = typename atomic_entry_t::value_type;

 private:
  static_assert(lockfree_mpmc_queue_entry<entry_t, value_t, index_t>);

 public:
  constexpr explicit lockfree_mpmc_queue(allocator_t allocator = allocator_t())
    requires(Capacity != utils::kRuntimeDefinedCapacity<index_t> &&
             utils::is_power_of_two(Capacity) && Capacity >= 2)
      : allocator_(std::move(allocator)) {
    if (!(ring_buffer_ = allocator_.allocate(Capacity))) [[unlikely]] {
      throw std::runtime_error("failed to allocate memory");
    }

    for (index_t i = 0; i < *capacity_; ++i) {
      entry_t entry{};
      entry.seqnum() = (i << 1);

      ring_buffer_[i].store(entry, std::memory_order::relaxed);
    }
  }

  constexpr explicit lockfree_mpmc_queue(index_t capacity,
                                         allocator_t allocator = allocator_t())
    requires(Capacity == utils::kRuntimeDefinedCapacity<index_t>)
      : allocator_(std::move(allocator)), capacity_(capacity) {
    if (!(utils::is_power_of_two(*capacity_) && *capacity_ >= 2)) {
      throw std::invalid_argument(
          "capacity must be a power of two greater than one");
    }

    if (!(ring_buffer_ = allocator_.allocate(*capacity_))) [[unlikely]] {
      throw std::runtime_error("failed to allocate memory");
    }

    for (index_t i = 0; i < *capacity_; ++i) {
      entry_t entry{};
      entry.seqnum() = (i << 1);

      ring_buffer_[i].store(entry, std::memory_order::relaxed);
    }
  }

  /**
   * @warning Destructor is only safe when called while no other thread is doing
   * operations on the queue
   *
   * is_trivially_destructible_v allows not to call std::destroy_at on queue
   * elements
   */
  constexpr ~lockfree_mpmc_queue() noexcept {
    allocator_.deallocate(ring_buffer_, *capacity_);
  }

 public:
  constexpr bool try_push(value_t value) noexcept {
    index_t push_to = push_to_.load(std::memory_order::relaxed);
    entry_t entry = ring_buffer_[push_to & (*capacity_ - 1)].load(
        std::memory_order::acquire);

    if (entry.seqnum() == (push_to << 1)) {
      entry_t desired{};
      std::construct_at(&desired.value(), std::move(value));
      desired.seqnum() = ((push_to << 1) | 1);

      if (ring_buffer_[push_to & (*capacity_ - 1)].compare_exchange_strong(
              entry, desired, std::memory_order::release,
              std::memory_order::acquire)) {
        push_to_.compare_exchange_strong(push_to, push_to + 1,
                                         std::memory_order::release,
                                         std::memory_order::acquire);
        return true;
      }
    } else if (entry.seqnum() == ((push_to << 1) | 1) ||
               entry.seqnum() == ((push_to + *capacity_) << 1)) {
      push_to_.compare_exchange_strong(push_to, push_to + 1,
                                       std::memory_order::release,
                                       std::memory_order::acquire);
    }
    return false;
  }

  constexpr void push(value_t value) noexcept { while (!try_push(value)); }

  constexpr bool try_pop(value_t& value) noexcept {
    index_t pop_from = pop_from_.load(std::memory_order::relaxed);
    entry_t entry = ring_buffer_[pop_from & (*capacity_ - 1)].load(
        std::memory_order::acquire);

    if (entry.seqnum() == ((pop_from << 1) | 1)) {
      entry_t desired{};
      std::construct_at(&desired.value(), value_t{});
      desired.seqnum() = ((pop_from + *capacity_) << 1);

      if (ring_buffer_[pop_from & (*capacity_ - 1)].compare_exchange_strong(
              entry, desired, std::memory_order::release,
              std::memory_order::acquire)) {
        value = std::move(entry.value());
        pop_from_.compare_exchange_strong(pop_from, pop_from + 1,
                                          std::memory_order::release,
                                          std::memory_order::acquire);
        return true;
      }
    } else if ((entry.seqnum() | 1) == (((pop_from + *capacity_) << 1) | 1)) {
      pop_from_.compare_exchange_strong(pop_from, pop_from + 1,
                                        std::memory_order::release,
                                        std::memory_order::acquire);
    }
    return false;
  }

  constexpr value_t pop() noexcept {
    value_t value;
    while (!try_pop(value));
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

 private:
  atomic_entry_t* ring_buffer_ = nullptr;

  alignas(utils::kCacheLineSize) std::atomic<index_t> push_to_ = 0;
  alignas(utils::kCacheLineSize) std::atomic<index_t> pop_from_ = 0;

  [[no_unique_address]] allocator_t allocator_;
  [[no_unique_address]] const utils::conditionally_runtime<
      index_t, Capacity == utils::kRuntimeDefinedCapacity<index_t>, Capacity>
      capacity_;
};

}  // namespace core::queues
