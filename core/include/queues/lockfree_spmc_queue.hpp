#pragma once

#include <atomic>
#include <memory>
#include <new>
#include <stdexcept>

#include "utils/conditionally_runtime.hpp"
#include "utils/dynamic_extent.hpp"
#include "utils/mixins.hpp"
#include "utils/predicates.hpp"

namespace core::queues {

namespace concepts {

/**
 * @brief Queue entry requirements
 *
 * Requires empty() to reduce contention on the queue indices by increasing it
 * on the queue elements
 */
template <typename T, typename Value>
concept lockfree_spmc_queue_entry = requires(T entry) {
  { entry.value() } -> std::same_as<Value&>;
  { entry.empty() } -> std::same_as<std::atomic<bool>&>;

  requires noexcept(entry.value());
  requires noexcept(entry.empty());

  requires std::is_nothrow_destructible_v<Value>;
  requires std::is_nothrow_move_constructible_v<Value>;
  requires std::is_nothrow_move_assignable_v<Value>;
};

template <typename T, typename Value>
concept lockfree_spmc_queue_allocator = requires {
  requires lockfree_spmc_queue_entry<typename T::value_type, Value>;
};

}  // namespace concepts

namespace detail {

/**
 * @brief Default queue entry only for internal usage
 */
template <typename Value,
          std::size_t Alignment = alignof(std::pair<Value, bool>)>
class alignas(Alignment) lockfree_spmc_queue_entry final {
 public:
  constexpr Value& value() noexcept { return value_; }
  constexpr std::atomic<bool>& empty() noexcept { return empty_; }

 private:
  Value value_;
  std::atomic<bool> empty_;
};

}  // namespace detail

/**
 * @brief Threadsafe queue for 1 producer and many consumers
 */
template <typename Value, std::unsigned_integral Index = std::size_t,
          Index Capacity = utils::kDynamicExtent<Index>,
          concepts::lockfree_spmc_queue_allocator<Value> Allocator =
              std::allocator<detail::lockfree_spmc_queue_entry<Value>>>
class lockfree_spmc_queue final : public utils::non_copyable,
                                  public utils::non_movable {
 public:
  using value_t = Value;
  using index_t = Index;
  using allocator_t = Allocator;
  using allocator_value_t = typename Allocator::value_type;

 public:
  constexpr explicit lockfree_spmc_queue(
      const allocator_t& allocator = allocator_t()) noexcept(false)
    requires(Capacity != utils::kDynamicExtent<index_t> &&
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
      index_t capacity,
      const allocator_t& allocator = allocator_t()) noexcept(false)
    requires(Capacity == utils::kDynamicExtent<index_t>)
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
    clear();
    allocator_.deallocate(ring_buffer_, *capacity_);
  }

 public:
  template <typename... Args>
  constexpr bool try_emplace(Args&&... args) noexcept
    requires(std::is_nothrow_constructible_v<Value, Args...>)
  {
    allocator_value_t& entry = ring_buffer_[push_to_ & (*capacity_ - 1)];
    if (!entry.empty().load(std::memory_order::acquire)) {
      return false;
    }

    std::construct_at(&entry.value(), std::forward<Args>(args)...);

    push_to_ += 1;
    entry.empty().store(false, std::memory_order::release);
    available_to_push_.fetch_sub(1, std::memory_order::relaxed);
    return true;
  }

  constexpr bool try_push(value_t value) noexcept {
    return try_emplace(std::move(value));
  }

  template <typename... Args>
  constexpr void emplace(Args&&... args) noexcept
    requires(std::is_nothrow_constructible_v<Value, Args...>)
  {
    allocator_value_t& entry = ring_buffer_[push_to_ & (*capacity_ - 1)];
    entry.empty().wait(false, std::memory_order::acquire);

    std::construct_at(&entry.value(), std::forward<Args>(args)...);

    push_to_ += 1;
    entry.empty().store(false, std::memory_order::release);
    available_to_push_.fetch_sub(1, std::memory_order::relaxed);
  }

  constexpr void push(value_t value) noexcept { emplace(std::move(value)); }

  constexpr bool try_pop(value_t& value) noexcept {
    const index_t available_to_push =
        available_to_push_.fetch_add(1, std::memory_order::relaxed);
    if (available_to_push >= *capacity_) {
      available_to_push_.fetch_sub(1, std::memory_order::relaxed);
      return false;
    }

    const index_t pop_from = pop_from_.fetch_add(1, std::memory_order::relaxed);
    allocator_value_t& entry = ring_buffer_[pop_from & (*capacity_ - 1)];

    value = std::move(entry.value());
    std::destroy_at(&entry.value());

    entry.empty().store(true, std::memory_order::release);
    entry.empty().notify_one();
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
    allocator_value_t& entry = ring_buffer_[pop_from & (*capacity_ - 1)];

    value_t value(std::move(entry.value()));
    std::destroy_at(&entry.value());

    entry.empty().store(true, std::memory_order::release);
    entry.empty().notify_one();
    return value;
  }

 public:
  constexpr index_t capacity() const noexcept { return *capacity_; }

  /**
   * @brief Approximate size
   */
  constexpr index_t size() const noexcept {
    return *capacity_ - available_to_push_.load(std::memory_order::relaxed);
  }

 public:
  const allocator_t get_allocator() const noexcept { return allocator_; }

 private:
  allocator_value_t* ring_buffer_ = nullptr;

  alignas(std::hardware_destructive_interference_size)
      std::atomic<index_t> available_to_push_;
  alignas(std::hardware_destructive_interference_size) index_t push_to_ = 0;
  alignas(std::hardware_destructive_interference_size)
      std::atomic<index_t> pop_from_ = 0;

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
      const index_t available_to_push =
          available_to_push_.fetch_add(1, std::memory_order::relaxed);
      if (available_to_push >= *capacity_) {
        available_to_push_.fetch_sub(1, std::memory_order::relaxed);
        break;
      }

      const index_t pop_from =
          pop_from_.fetch_add(1, std::memory_order::relaxed);
      allocator_value_t& entry = ring_buffer_[pop_from & (*capacity_ - 1)];

      std::destroy_at(&entry.value());

      entry.empty().store(true, std::memory_order::release);
      entry.empty().notify_one();
    }
  }
};

}  // namespace core::queues
