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
 * Requires empty() to reduce contention on the queue indices by increasing it
 * on the queue elements
 */
template <typename T, typename Value>
concept lockfree_spmc_queue_entry = requires(T entry) {
  { entry.value() } -> std::same_as<Value&>;
  { entry.empty() } -> std::same_as<bool&>;
};

namespace detail {

/**
 * @brief Default queue entry only for internal usage
 */
template <typename Value>
class lockfree_spmc_queue_entry_impl final {
 private:
  Value value_;
  bool empty_;

 public:
  constexpr Value& value() { return value_; }
  constexpr bool& empty() { return empty_; }
};

}  // namespace detail

/**
 * @brief Threadsafe queue for 1 producer and many consumers
 */
template <typename Value, std::unsigned_integral Index = std::size_t,
          Index Capacity = utils::kRuntimeDefinedCapacity<Index>,
          typename Allocator =
              std::allocator<detail::lockfree_spmc_queue_entry_impl<Value>>>
class lockfree_spmc_queue final : utils::non_copyable, utils::non_movable {
 public:
  using value_t = Value;
  using index_t = Index;
  using allocator_t = Allocator;
  using entry_t = typename Allocator::value_type;

 private:
  static_assert(std::is_nothrow_default_constructible_v<value_t>);
  static_assert(std::is_nothrow_destructible_v<value_t>);
  static_assert(std::is_nothrow_move_constructible_v<value_t>);
  static_assert(std::is_nothrow_move_assignable_v<value_t>);
  static_assert(lockfree_spmc_queue_entry<entry_t, value_t>);

 public:
  constexpr explicit lockfree_spmc_queue(
      const allocator_t& allocator = allocator_t())
    requires(Capacity != utils::kRuntimeDefinedCapacity<index_t> &&
             utils::is_power_of_two(Capacity) && Capacity > 1)
      : available_to_push_(Capacity), allocator_(allocator) {
    if (!(ring_buffer_ = allocator_.allocate(Capacity))) [[unlikely]] {
      throw std::runtime_error("failed to allocate memory");
    }

    for (index_t i = 0; i < *capacity_; ++i) {
      ring_buffer_[i].empty() = true;
    }
  }

  constexpr explicit lockfree_spmc_queue(
      index_t capacity, const allocator_t& allocator = allocator_t())
    requires(Capacity == utils::kRuntimeDefinedCapacity<index_t>)
      : available_to_push_(capacity),
        allocator_(allocator),
        capacity_(capacity) {
    if (!utils::is_power_of_two(*capacity_) || *capacity_ <= 1) {
      throw std::invalid_argument(
          "capacity must be a power of two greater than one");
    }

    if (!(ring_buffer_ = allocator_.allocate(*capacity_))) [[unlikely]] {
      throw std::runtime_error("failed to allocate memory");
    }

    for (index_t i = 0; i < *capacity_; ++i) {
      ring_buffer_[i].empty() = true;
    }
  }

  /**
   * @warning Destructor is only safe when called while no other thread is doing
   * operations on the queue
   */
  constexpr ~lockfree_spmc_queue() noexcept {
    value_t value;
    while (try_pop(value));
    allocator_.deallocate(ring_buffer_, *capacity_);
  }

 public:
  constexpr bool try_push(value_t value) noexcept {
    entry_t& entry = ring_buffer_[push_to_ & (*capacity_ - 1)];
    if (!reinterpret_cast<std::atomic<bool>&>(entry.empty())
             .load(std::memory_order::acquire)) {
      return false;
    }

    std::construct_at(&entry.value(), std::move(value));

    push_to_ += 1;
    reinterpret_cast<std::atomic<bool>&>(entry.empty())
        .store(false, std::memory_order::release);
    available_to_push_.fetch_sub(1, std::memory_order::relaxed);
    return true;
  }

  constexpr void push(value_t value) noexcept {
    entry_t& entry = ring_buffer_[push_to_ & (*capacity_ - 1)];
    reinterpret_cast<std::atomic<bool>&>(entry.empty())
        .wait(false, std::memory_order::acquire);

    std::construct_at(&entry.value(), std::move(value));

    push_to_ += 1;
    reinterpret_cast<std::atomic<bool>&>(entry.empty())
        .store(false, std::memory_order::release);
    available_to_push_.fetch_sub(1, std::memory_order::relaxed);
  }

  constexpr bool try_pop(value_t& value) noexcept {
    const index_t available_to_push =
        available_to_push_.fetch_add(1, std::memory_order::relaxed);
    if (available_to_push >= *capacity_) {
      available_to_push_.fetch_sub(1, std::memory_order::relaxed);
      return false;
    }

    const index_t pop_from = pop_from_.fetch_add(1, std::memory_order::relaxed);
    entry_t& entry = ring_buffer_[pop_from & (*capacity_ - 1)];

    value = std::move(entry.value());
    std::destroy_at(&entry.value());

    reinterpret_cast<std::atomic<bool>&>(entry.empty())
        .store(true, std::memory_order::release);
    reinterpret_cast<std::atomic<bool>&>(entry.empty()).notify_one();
    return true;
  }

  constexpr value_t pop() noexcept {
    const index_t available_to_push =
        available_to_push_.fetch_add(1, std::memory_order::relaxed);
    if (available_to_push >= *capacity_) {
      while (available_to_push_.load(std::memory_order::acquire) >=
             available_to_push + 1);
    }

    const index_t pop_from = pop_from_.fetch_add(1, std::memory_order::relaxed);
    entry_t& entry = ring_buffer_[pop_from & (*capacity_ - 1)];

    value_t value(std::move(entry.value()));
    std::destroy_at(&entry.value());

    reinterpret_cast<std::atomic<bool>&>(entry.empty())
        .store(true, std::memory_order::release);
    reinterpret_cast<std::atomic<bool>&>(entry.empty()).notify_one();
    return value;
  }

 public:
  constexpr index_t capacity() const noexcept { return *capacity_; }

 private:
  entry_t* ring_buffer_ = nullptr;

  alignas(utils::kCacheLineSize) std::atomic<index_t> available_to_push_;
  alignas(utils::kCacheLineSize) index_t push_to_ = 0;
  alignas(utils::kCacheLineSize) std::atomic<index_t> pop_from_ = 0;

  [[no_unique_address]] allocator_t allocator_;
  [[no_unique_address]] const utils::conditionally_runtime<
      index_t, Capacity == utils::kRuntimeDefinedCapacity<index_t>, Capacity>
      capacity_;
};

}  // namespace core::queues
