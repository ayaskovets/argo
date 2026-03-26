#pragma once

#include <swiss/conditionally_runtime.hpp>
#include <swiss/dynamic_extent.hpp>
#include <swiss/mixins.hpp>
#include <swiss/predicates.hpp>

#include <atomic>
#include <memory>
#include <new>
#include <stdexcept>

namespace wip::queues {

namespace concepts {

/**
 * @brief Queue entry requirements
 *
 * Requires seqnum() to avoid the ABA problem
 */
template <typename T, typename Value, typename Index>
concept lockfree_mpmc_queue_entry = requires(T entry) {
  { entry.value() } -> std::same_as<Value&>;
  requires noexcept(entry.value());

  { entry.seqnum() } -> std::same_as<Index&>;
  requires noexcept(entry.seqnum());

  requires std::is_nothrow_default_constructible_v<T>;
  requires std::is_trivially_destructible_v<T>;
  requires std::is_nothrow_destructible_v<T>;
  requires std::is_trivially_copy_constructible_v<T>;
  requires std::is_nothrow_copy_constructible_v<T>;
  requires std::is_trivially_move_assignable_v<T>;
  requires std::is_nothrow_move_assignable_v<T>;

  requires std::unsigned_integral<Index>;
};

template <typename T, typename Value, typename Index>
concept lockfree_mpmc_queue_allocator = requires {
  requires std::is_same_v<typename T::value_type,
                          std::atomic<typename T::value_type::value_type>>;
  requires T::value_type::is_always_lock_free;
  requires lockfree_mpmc_queue_entry<typename T::value_type::value_type, Value,
                                     Index>;
};

}  // namespace concepts

namespace detail {

/**
 * @brief Default queue entry only for internal usage
 */
template <typename Value, std::unsigned_integral Index = std::size_t,
          std::size_t Alignment = alignof(std::pair<Value, Index>)>
class alignas(Alignment) lockfree_mpmc_queue_entry final {
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
          Index Capacity = swiss::kDynamicExtent<Index>,
          concepts::lockfree_mpmc_queue_allocator<Value, Index> Allocator =
              std::allocator<
                  std::atomic<detail::lockfree_mpmc_queue_entry<Value, Index>>>>
class lockfree_mpmc_queue final : public swiss::non_copyable,
                                  public swiss::non_movable {
 private:
  using allocator_underlying_value_t =
      typename Allocator::value_type::value_type;

 public:
  using value_type = Value;
  using index_type = Index;
  using allocator_type = Allocator;
  using allocator_value_type = typename Allocator::value_type;

 public:
  constexpr explicit lockfree_mpmc_queue(
      const allocator_type& allocator = allocator_type()) noexcept(false)
    requires(Capacity != swiss::kDynamicExtent<index_type>)
      : allocator_(allocator) {
    static_assert(swiss::is_power_of_two(Capacity) && Capacity >= 2,
                  "capacity must be a power of two greater than one");

    if (!(ring_buffer_ = allocator_.allocate(*capacity_))) [[unlikely]] {
      throw std::runtime_error("failed to allocate memory");
    }

    for (index_type i = 0; i < *capacity_; ++i) {
      allocator_underlying_value_t entry{};
      entry.seqnum() = (i << 1);

      ring_buffer_[i].store(entry, std::memory_order::relaxed);
    }
  }

  constexpr explicit lockfree_mpmc_queue(
      index_type capacity,
      const allocator_type& allocator = allocator_type()) noexcept(false)
    requires(Capacity == swiss::kDynamicExtent<index_type>)
      : allocator_(allocator), capacity_(capacity) {
    if (!(swiss::is_power_of_two(*capacity_) && *capacity_ >= 2)) {
      throw std::invalid_argument(
          "capacity must be a power of two greater than one");
    }

    if (!(ring_buffer_ = allocator_.allocate(*capacity_))) [[unlikely]] {
      throw std::runtime_error("failed to allocate memory");
    }

    for (index_type i = 0; i < *capacity_; ++i) {
      allocator_underlying_value_t entry{};
      entry.seqnum() = (i << 1);

      ring_buffer_[i].store(entry, std::memory_order::relaxed);
    }
  }

  /**
   * @warning Destructor operations are not thread safe and it is assumed that
   * there is only one thread doing any operations on the queue at the moment
   * destruction occurs
   *
   * is_trivially_destructible_v allows not to call std::destroy_at on queue
   * elements
   */
  constexpr ~lockfree_mpmc_queue() noexcept {
    allocator_.deallocate(ring_buffer_, *capacity_);
  }

 public:
  template <typename... Args>
  constexpr bool try_emplace(Args&&... args) noexcept
    requires(std::is_nothrow_constructible_v<Value, Args...>)
  {
    index_type push_to = push_to_.load(std::memory_order::relaxed);
    allocator_underlying_value_t entry =
        ring_buffer_[push_to & (*capacity_ - 1)].load(
            std::memory_order::acquire);

    if (entry.seqnum() == (push_to << 1)) {
      allocator_underlying_value_t desired{};
      std::construct_at(&desired.value(), std::forward<Args>(args)...);
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

  constexpr bool try_push(value_type value) noexcept {
    return try_emplace(std::move(value));
  }

  template <typename... Args>
  constexpr void emplace(Args&&... args) noexcept
    requires(std::is_nothrow_constructible_v<Value, Args...>)
  {
    while (!try_emplace(std::forward<Args>(args)...));
  }

  constexpr void push(value_type value) noexcept { emplace(std::move(value)); }

  constexpr bool try_pop(value_type& value) noexcept {
    index_type pop_from = pop_from_.load(std::memory_order::relaxed);
    allocator_underlying_value_t entry =
        ring_buffer_[pop_from & (*capacity_ - 1)].load(
            std::memory_order::acquire);

    if (entry.seqnum() == ((pop_from << 1) | 1)) {
      allocator_underlying_value_t desired{};
      std::construct_at(&desired.value(), value_type{});
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

  constexpr value_type pop() noexcept {
    value_type value;
    while (!try_pop(value));
    return value;
  }

 public:
  constexpr index_type capacity() const noexcept { return *capacity_; }

  /**
   * @brief Approximate size
   */
  constexpr index_type size() const noexcept {
    index_type const push_to = push_to_.load(std::memory_order::relaxed);
    index_type const pop_from = pop_from_.load(std::memory_order::relaxed);

    return push_to >= pop_from ? push_to - pop_from
                               : *capacity_ - pop_from - push_to + 1;
  }

 public:
  allocator_type get_allocator() const noexcept { return allocator_; }

 private:
  allocator_value_type* ring_buffer_ = nullptr;

  alignas(std::hardware_destructive_interference_size)
      std::atomic<index_type> push_to_ = 0;
  alignas(std::hardware_destructive_interference_size)
      std::atomic<index_type> pop_from_ = 0;

  [[no_unique_address]] allocator_type allocator_;
  [[no_unique_address]] swiss::conditionally_runtime<
      index_type, Capacity == swiss::kDynamicExtent<index_type>, Capacity> const
      capacity_;
};

}  // namespace wip::queues
