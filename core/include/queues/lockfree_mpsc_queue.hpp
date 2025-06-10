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

template <typename T, typename Value>
concept lockfree_mpsc_queue_entry = requires(T entry) {
  { entry.value() } -> std::same_as<Value&>;
  { entry.empty() } -> std::same_as<bool&>;
};

}  // namespace detail

template <typename Value>
class lockfree_mpsc_queue_entry final {
 private:
  Value value_;
  bool empty_;

 public:
  constexpr Value& value() { return value_; }
  constexpr bool& empty() { return empty_; }
};

template <typename Value, std::unsigned_integral Index = std::size_t,
          Index Capacity = utils::kDynamicCapacity<Index>,
          typename Allocator = std::allocator<lockfree_mpsc_queue_entry<Value>>>
class lockfree_mpsc_queue final : utils::non_copyable, utils::non_movable {
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
  static_assert(detail::lockfree_mpsc_queue_entry<entry_t, value_t>);

 public:
  constexpr explicit lockfree_mpsc_queue(
      const allocator_t& allocator = allocator_t())
    requires(Capacity != utils::kDynamicCapacity<index_t> &&
             utils::is_power_of_two(Capacity) && Capacity > 1)
      : allocator_(allocator) {
    if (!(ring_buffer_ = allocator_.allocate(Capacity))) [[unlikely]] {
      throw std::runtime_error("failed to allocate memory");
    }

    for (index_t i = 0; i < *capacity_; ++i) {
      ring_buffer_[i].empty() = true;
    }
  }

  constexpr explicit lockfree_mpsc_queue(
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
      ring_buffer_[i].empty() = true;
    }
  }

  constexpr ~lockfree_mpsc_queue() noexcept {
    value_t value;
    while (try_pop(value));
    allocator_.deallocate(ring_buffer_, *capacity_);
  }

 public:
  constexpr bool try_push(value_t value) noexcept {
    const index_t size = size_.fetch_add(1, std::memory_order::relaxed);
    if (size >= *capacity_) {
      size_.fetch_sub(1, std::memory_order::relaxed);
      return false;
    }

    const index_t push_to = push_to_.fetch_add(1, std::memory_order::relaxed);
    entry_t& entry = ring_buffer_[push_to & (*capacity_ - 1)];

    std::construct_at(&entry.value(), std::move(value));
    reinterpret_cast<std::atomic<bool>&>(entry.empty())
        .store(false, std::memory_order::release);
    reinterpret_cast<std::atomic<bool>&>(entry.empty()).notify_one();
    return true;
  }

  constexpr void push(value_t value) noexcept {
    const index_t size = size_.fetch_add(1, std::memory_order::relaxed);
    if (size >= *capacity_) {
      while (size_.load(std::memory_order::acquire) >= size + 1);
    }

    const index_t push_to = push_to_.fetch_add(1, std::memory_order::relaxed);
    entry_t& entry = ring_buffer_[push_to & (*capacity_ - 1)];

    std::construct_at(&entry.value(), std::move(value));
    reinterpret_cast<std::atomic<bool>&>(entry.empty())
        .store(false, std::memory_order::release);
    reinterpret_cast<std::atomic<bool>&>(entry.empty()).notify_one();
    return;
  }

  constexpr bool try_pop(value_t& value) noexcept {
    entry_t& entry = ring_buffer_[pop_from_ & (*capacity_ - 1)];
    if (reinterpret_cast<std::atomic<bool>&>(entry.empty())
            .load(std::memory_order::acquire)) {
      return false;
    }

    value = std::move(entry.value());
    std::destroy_at(&entry.value());

    pop_from_ += 1;
    reinterpret_cast<std::atomic<bool>&>(entry.empty())
        .store(true, std::memory_order::release);
    size_.fetch_sub(1, std::memory_order::relaxed);
    return true;
  }

  constexpr value_t pop() noexcept {
    entry_t& entry = ring_buffer_[pop_from_ & (*capacity_ - 1)];
    reinterpret_cast<std::atomic<bool>&>(entry.empty())
        .wait(true, std::memory_order::acquire);

    value_t value(std::move(entry.value()));
    std::destroy_at(&entry.value());

    pop_from_ += 1;
    reinterpret_cast<std::atomic<bool>&>(entry.empty())
        .store(true, std::memory_order::release);
    size_.fetch_sub(1, std::memory_order::relaxed);
    return value;
  }

 public:
  constexpr index_t capacity() const noexcept { return *capacity_; }

 private:
  entry_t* ring_buffer_ = nullptr;

  alignas(utils::kCacheLineSize) std::atomic<index_t> size_ = 0;
  alignas(utils::kCacheLineSize) std::atomic<index_t> push_to_ = 0;
  alignas(utils::kCacheLineSize) index_t pop_from_ = 0;

  [[no_unique_address]] allocator_t allocator_;
  [[no_unique_address]] const utils::conditionally_runtime<
      index_t, Capacity == utils::kDynamicCapacity<index_t>, Capacity>
      capacity_;
};

}  // namespace core::queues
