#pragma once

#include <atomic>
#include <memory>
#include <new>
#include <stdexcept>

#include "utils/conditionally_runtime.hpp"
#include "utils/constants.hpp"
#include "utils/mixins.hpp"
#include "utils/predicates.hpp"

namespace core::queues {

namespace concepts {

/**
 * @brief Queue entry requirements
 *
 * Requires seqnum() to reduce the ABA problem
 */
template <typename T, typename Value, typename Index>
concept lockless_mpmc_queue_entry = requires(T entry) {
  { entry.value() } -> std::same_as<Value&>;
  { entry.seqnum() } -> std::same_as<std::atomic<Index>&>;

  requires noexcept(entry.value());
  requires noexcept(entry.seqnum());

  requires std::is_nothrow_destructible_v<Value>;
  requires std::is_nothrow_move_constructible_v<Value>;
  requires std::is_nothrow_move_assignable_v<Value>;

  requires std::unsigned_integral<Index>;
};

template <typename T, typename Value, typename Index>
concept lockless_mpmc_queue_allocator = requires {
  requires lockless_mpmc_queue_entry<typename T::value_type, Value, Index>;
};

}  // namespace concepts

namespace detail {

/**
 * @brief Default queue entry only for internal usage
 */
template <typename Value, std::unsigned_integral Index = std::size_t,
          std::size_t Alignment = alignof(std::pair<Value, Index>)>
class alignas(Alignment) lockless_mpmc_queue_entry_impl final {
 public:
  constexpr Value& value() noexcept { return value_; }
  constexpr std::atomic<Index>& seqnum() noexcept { return seqnum_; }

 private:
  Value value_;
  std::atomic<Index> seqnum_;
};

}  // namespace detail

/**
 * @brief Threadsafe queue for many producers and many consumers
 *
 * @warning This queue does not handle integer overflow well, but using 64-bit
 * unsigned integers as indices makes the problem virtually nonexistent because
 * even handling 1B messages per second overflow would occur once every 584
 * years (for Index = std::size_t)
 *
 * Implemented as a ring buffer with a sequence number for each element to
 * reduce the risk of the ABA problem when Index is a large enough type for
 * rollover to happen rarely
 */
template <
    typename Value, std::unsigned_integral Index = std::size_t,
    Index Capacity = utils::kRuntimeDefinedCapacity<Index>,
    concepts::lockless_mpmc_queue_allocator<Value, Index> Allocator =
        std::allocator<detail::lockless_mpmc_queue_entry_impl<Value, Index>>>
class lockless_mpmc_queue final : utils::non_copyable, utils::non_movable {
 public:
  using value_t = Value;
  using index_t = Index;
  using allocator_t = Allocator;
  using allocator_value_t = typename Allocator::value_type;

 public:
  constexpr explicit lockless_mpmc_queue(
      const allocator_t& allocator = allocator_t())
    requires(Capacity != utils::kRuntimeDefinedCapacity<index_t> &&
             utils::is_power_of_two(Capacity) && Capacity > 1)
      : allocator_(allocator) {
    if (!(ring_buffer_ = allocator_.allocate(Capacity))) [[unlikely]] {
      throw std::runtime_error("failed to allocate memory");
    }

    for (index_t i = 0; i < *capacity_; ++i) {
      ring_buffer_[i].seqnum() = i;
    }
  }

  constexpr explicit lockless_mpmc_queue(
      index_t capacity, const allocator_t& allocator = allocator_t())
    requires(Capacity == utils::kRuntimeDefinedCapacity<index_t>)
      : allocator_(allocator), capacity_(capacity) {
    if (!utils::is_power_of_two(*capacity_) || *capacity_ <= 1) {
      throw std::invalid_argument(
          "capacity must be a power of two greater than one");
    }

    if (!(ring_buffer_ = allocator_.allocate(*capacity_))) [[unlikely]] {
      throw std::runtime_error("failed to allocate memory");
    }

    for (index_t i = 0; i < *capacity_; ++i) {
      ring_buffer_[i].seqnum() = i;
    }
  }

  /**
   * @warning Destructor is only safe when called while no other thread is doing
   * operations on the queue
   */
  constexpr ~lockless_mpmc_queue() noexcept {
    clear();
    allocator_.deallocate(ring_buffer_, *capacity_);
  }

 public:
  template <typename... Args>
  constexpr bool try_emplace(Args&&... args) noexcept
    requires(std::is_nothrow_constructible_v<Value, Args...>)
  {
    index_t push_to = push_to_.load(std::memory_order::relaxed);
    allocator_value_t& entry = ring_buffer_[push_to & (*capacity_ - 1)];

    if (push_to != entry.seqnum().load(std::memory_order::acquire)) {
      return false;
    }
    if (!push_to_.compare_exchange_strong(push_to, push_to + 1,
                                          std::memory_order::relaxed)) {
      return false;
    }

    std::construct_at(&entry.value(), std::forward<Args>(args)...);

    entry.seqnum().store(push_to + 1, std::memory_order::release);
    return true;
  }

  constexpr bool try_push(value_t value) noexcept {
    return try_emplace(std::move(value));
  }

  template <typename... Args>
  constexpr void emplace(Args&&... args) noexcept
    requires(std::is_nothrow_constructible_v<Value, Args...>)
  {
    do {
      index_t push_to = push_to_.load(std::memory_order::relaxed);
      allocator_value_t& entry = ring_buffer_[push_to & (*capacity_ - 1)];
      if (push_to != entry.seqnum().load(std::memory_order::acquire)) {
        continue;
      }

      if (push_to_.compare_exchange_weak(push_to, push_to + 1,
                                         std::memory_order::relaxed)) {
        std::construct_at(&entry.value(), std::forward<Args>(args)...);
        entry.seqnum().store(push_to + 1, std::memory_order::release);
        break;
      }
    } while (true);
  }

  constexpr void push(value_t value) noexcept { emplace(std::move(value)); }

  constexpr bool try_pop(value_t& value) noexcept {
    index_t pop_from = pop_from_.load(std::memory_order::relaxed);
    allocator_value_t& entry = ring_buffer_[pop_from & (*capacity_ - 1)];

    if (pop_from + 1 != entry.seqnum().load(std::memory_order::acquire)) {
      return false;
    }
    if (!pop_from_.compare_exchange_strong(pop_from, pop_from + 1,
                                           std::memory_order::relaxed)) {
      return false;
    }

    value = std::move(entry.value());
    std::destroy_at(&entry.value());

    entry.seqnum().store(pop_from + *capacity_, std::memory_order::release);
    return true;
  }

  constexpr value_t pop() noexcept {
    do {
      index_t pop_from = pop_from_.load(std::memory_order::relaxed);
      allocator_value_t& entry = ring_buffer_[pop_from & (*capacity_ - 1)];
      if (pop_from + 1 != entry.seqnum().load(std::memory_order::acquire)) {
        continue;
      }

      if (pop_from_.compare_exchange_weak(pop_from, pop_from + 1,
                                          std::memory_order::relaxed)) {
        value_t value(std::move(entry.value()));
        std::destroy_at(&entry.value());

        entry.seqnum().store(pop_from + *capacity_, std::memory_order::release);
        return value;
      }
    } while (true);
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
  alignas(std::hardware_destructive_interference_size)
      std::atomic<index_t> pop_from_ = 0;

  [[no_unique_address]] allocator_t allocator_;
  [[no_unique_address]] const utils::conditionally_runtime<
      index_t, Capacity == utils::kRuntimeDefinedCapacity<index_t>, Capacity>
      capacity_;

 private:
  /**
   * @brief Preserves queue invariants. Assumes that no other thread is doing
   * any operations on the queue
   */
  constexpr void clear() noexcept {
    while (true) {
      index_t pop_from = pop_from_.load(std::memory_order::relaxed);
      allocator_value_t& entry = ring_buffer_[pop_from & (*capacity_ - 1)];

      if (pop_from + 1 != entry.seqnum().load(std::memory_order::acquire)) {
        break;
      }
      if (!pop_from_.compare_exchange_strong(pop_from, pop_from + 1,
                                             std::memory_order::relaxed)) {
        break;
      }

      std::destroy_at(&entry.value());

      entry.seqnum().store(pop_from + *capacity_, std::memory_order::release);
    }
  }
};

}  // namespace core::queues
