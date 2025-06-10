#pragma once

#include <atomic>
#include <optional>
#include <type_traits>

#include "utils/conditionally_runtime.hpp"
#include "utils/constants.hpp"
#include "utils/mixins.hpp"
#include "utils/predicates.hpp"

namespace core::queues {

namespace detail {

template <typename T, typename Value, typename Index>
concept lockless_spmc_queue_entry = requires(T entry) {
  { entry.value() } -> std::same_as<Value&>;
  { entry.seqnum() } -> std::same_as<Index&>;
};

}  // namespace detail

template <typename Value, typename Index>
class lockless_spmc_queue_entry final {
 private:
  Value value_;
  Index seqnum_;

 public:
  constexpr Value& value() { return value_; }
  constexpr Index& seqnum() { return seqnum_; }
};

// NOTE: this queue does not handle push & pop indices overflow well, but using
// 64-bit unsigned integers as indices makes the problem pretty much
// non-existent because even handling 1B messages per second overflow would
// happed once in 584 years (for Index = std::size_t)
template <typename Value, std::unsigned_integral Index = std::size_t,
          Index Capacity = utils::kDynamicCapacity<Index>,
          typename Allocator =
              std::allocator<lockless_spmc_queue_entry<Value, Index>>>
class lockless_mpmc_queue final : utils::non_copyable, utils::non_movable {
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
  static_assert(detail::lockless_spmc_queue_entry<entry_t, value_t, index_t>);

 public:
  constexpr explicit lockless_mpmc_queue(
      const allocator_t& allocator = allocator_t())
    requires(Capacity != utils::kDynamicCapacity<index_t> &&
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
    requires(Capacity == utils::kDynamicCapacity<index_t>)
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

  constexpr ~lockless_mpmc_queue() noexcept {
    value_t value;
    while (try_pop(value));
    allocator_.deallocate(ring_buffer_, *capacity_);
  }

 public:
  constexpr bool try_push(value_t value) noexcept {
    index_t push_to = push_to_.load(std::memory_order::relaxed);
    entry_t& entry = ring_buffer_[push_to & (*capacity_ - 1)];

    if (push_to != reinterpret_cast<std::atomic<index_t>&>(entry.seqnum())
                       .load(std::memory_order::acquire)) {
      return false;
    }
    if (!push_to_.compare_exchange_strong(push_to, push_to + 1,
                                          std::memory_order::relaxed)) {
      return false;
    }

    std::construct_at(&entry.value(), std::move(value));

    reinterpret_cast<std::atomic<index_t>&>(entry.seqnum())
        .store(push_to + 1, std::memory_order::release);
    return true;
  }

  constexpr void push(value_t value) noexcept {
    do {
      index_t push_to = push_to_.load(std::memory_order::relaxed);
      entry_t& entry = ring_buffer_[push_to & (*capacity_ - 1)];
      if (push_to != reinterpret_cast<std::atomic<index_t>&>(entry.seqnum())
                         .load(std::memory_order::acquire)) {
        continue;
      }

      if (push_to_.compare_exchange_weak(push_to, push_to + 1,
                                         std::memory_order::relaxed)) {
        std::construct_at(&entry.value(), std::move(value));
        reinterpret_cast<std::atomic<index_t>&>(entry.seqnum())
            .store(push_to + 1, std::memory_order::release);
        break;
      }
    } while (true);
  }

  constexpr bool try_pop(value_t& value) noexcept {
    index_t pop_from = pop_from_.load(std::memory_order::relaxed);
    entry_t& entry = ring_buffer_[pop_from & (*capacity_ - 1)];

    if (pop_from + 1 != reinterpret_cast<std::atomic<index_t>&>(entry.seqnum())
                            .load(std::memory_order::acquire)) {
      return false;
    }
    if (!pop_from_.compare_exchange_strong(pop_from, pop_from + 1,
                                           std::memory_order::relaxed)) {
      return false;
    }

    value = std::move(entry.value());
    std::destroy_at(&entry.value());

    reinterpret_cast<std::atomic<index_t>&>(entry.seqnum())
        .store(pop_from + *capacity_, std::memory_order::release);
    return true;
  }

  constexpr value_t pop() noexcept {
    do {
      index_t pop_from = pop_from_.load(std::memory_order::relaxed);
      entry_t& entry = ring_buffer_[pop_from & (*capacity_ - 1)];
      if (pop_from + 1 !=
          reinterpret_cast<std::atomic<index_t>&>(entry.seqnum())
              .load(std::memory_order::acquire)) {
        continue;
      }

      if (pop_from_.compare_exchange_weak(pop_from, pop_from + 1,
                                          std::memory_order::relaxed)) {
        value_t value(std::move(entry.value()));
        std::destroy_at(&entry.value());

        reinterpret_cast<std::atomic<index_t>&>(entry.seqnum())
            .store(pop_from + *capacity_, std::memory_order::release);
        return value;
      }
    } while (true);
  }

 public:
  constexpr index_t capacity() const noexcept { return *capacity_; }

 private:
  entry_t* ring_buffer_ = nullptr;

  alignas(utils::kCacheLineSize) std::atomic<index_t> push_to_ = 0;
  alignas(utils::kCacheLineSize) std::atomic<index_t> pop_from_ = 0;

  [[no_unique_address]] allocator_t allocator_;
  [[no_unique_address]] const utils::conditionally_runtime<
      index_t, Capacity == utils::kDynamicCapacity<index_t>, Capacity>
      capacity_;
};

}  // namespace core::queues
