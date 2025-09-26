#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <new>
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
concept locked_mpmc_queue_entry = requires(T entry) {
  { entry.value() } -> std::same_as<Value&>;

  requires noexcept(entry.value());

  requires std::is_nothrow_destructible_v<Value>;
  requires std::is_nothrow_move_constructible_v<Value>;
  requires std::is_nothrow_move_assignable_v<Value>;
};

template <typename T, typename Value>
concept locked_mpmc_queue_allocator = requires {
  requires locked_mpmc_queue_entry<typename T::value_type, Value>;
};

}  // namespace concepts

namespace detail {

/**
 * @brief Default queue entry only for internal usage
 */
template <typename Value, std::size_t Alignment = alignof(Value)>
class alignas(Alignment) locked_mpmc_queue_entry_impl final {
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
 *
 * Allocated buffer always contains one additional element to allow for an
 * efficient implementation of a ring buffer indexing
 */
template <typename Value, std::unsigned_integral Index = std::size_t,
          Index Capacity = utils::kDynamicExtent<Index>,
          concepts::locked_mpmc_queue_allocator<Value> Allocator =
              std::allocator<detail::locked_mpmc_queue_entry_impl<Value>>>
class locked_mpmc_queue final : utils::non_copyable, utils::non_movable {
 public:
  using value_t = Value;
  using index_t = Index;
  using allocator_t = Allocator;
  using allocator_value_t = typename Allocator::value_type;

 public:
  constexpr explicit locked_mpmc_queue(
      const allocator_t& allocator = allocator_t())
    requires(Capacity != utils::kDynamicExtent<index_t> && Capacity > 0)
      : size_(0), allocator_(allocator) {
    if (!(ring_buffer_ = allocator_.allocate(*capacity_ + 1))) [[unlikely]] {
      throw std::runtime_error("failed to allocate memory");
    }
  }

  constexpr explicit locked_mpmc_queue(
      index_t capacity, const allocator_t& allocator = allocator_t())
    requires(Capacity == utils::kDynamicExtent<index_t>)
      : size_(0), allocator_(allocator), capacity_(capacity) {
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
  ~locked_mpmc_queue() noexcept {
    clear();
    allocator_.deallocate(ring_buffer_, *capacity_ + 1);
  }

 public:
  template <typename... Args>
  bool try_emplace(Args&&... args) noexcept
    requires(std::is_nothrow_constructible_v<Value, Args...>)
  {
    if (size_.load(std::memory_order::relaxed) >= *capacity_) {
      return false;
    }
    std::unique_lock<std::mutex> lock(mutex_, std::defer_lock_t{});
    if (!lock.try_lock()) {
      return false;
    }
    const index_t next_push_to = (push_to_ == *capacity_) ? 0 : push_to_ + 1;
    if (next_push_to == pop_from_) {
      return false;
    }

    std::construct_at(&ring_buffer_[push_to_].value(),
                      std::forward<Args>(args)...);

    push_to_ = next_push_to;
    size_.fetch_add(1, std::memory_order::relaxed);
    pop_available_cv_.notify_one();
    return true;
  }

  constexpr bool try_push(value_t value) noexcept {
    return try_emplace(std::move(value));
  }

  template <typename... Args>
  void emplace(Args&&... args) noexcept
    requires(std::is_nothrow_constructible_v<Value, Args...>)
  {
    std::unique_lock<std::mutex> lock(mutex_);
    push_available_cv_.wait(lock, [this] {
      const index_t next_push_to = (push_to_ == *capacity_) ? 0 : push_to_ + 1;
      return next_push_to != pop_from_;
    });

    std::construct_at(&ring_buffer_[push_to_].value(),
                      std::forward<Args>(args)...);

    const index_t next_push_to = (push_to_ == *capacity_) ? 0 : push_to_ + 1;
    push_to_ = next_push_to;
    size_.fetch_add(1, std::memory_order::relaxed);
    pop_available_cv_.notify_one();
  }

  constexpr void push(value_t value) noexcept { emplace(std::move(value)); }

  bool try_pop(value_t& value) noexcept {
    if (size_.load(std::memory_order::relaxed) == 0) {
      return false;
    }
    std::unique_lock<std::mutex> lock(mutex_, std::defer_lock_t{});
    if (!lock.try_lock()) {
      return false;
    }
    if (pop_from_ == push_to_) {
      return false;
    }

    value = std::move(ring_buffer_[pop_from_].value());
    std::destroy_at(&ring_buffer_[pop_from_].value());

    const index_t next_pop_from = (pop_from_ == *capacity_) ? 0 : pop_from_ + 1;
    pop_from_ = next_pop_from;
    size_.fetch_sub(1, std::memory_order::relaxed);
    push_available_cv_.notify_one();
    return true;
  }

  value_t pop() noexcept {
    std::unique_lock<std::mutex> lock(mutex_);
    pop_available_cv_.wait(lock, [this] { return pop_from_ != push_to_; });

    value_t value(std::move(ring_buffer_[pop_from_].value()));
    std::destroy_at(&ring_buffer_[pop_from_].value());

    const index_t next_pop_from = (pop_from_ == *capacity_) ? 0 : pop_from_ + 1;
    pop_from_ = next_pop_from;

    size_.fetch_sub(1, std::memory_order::relaxed);
    push_available_cv_.notify_one();
    return value;
  }

 public:
  constexpr index_t capacity() const noexcept { return *capacity_; }
  constexpr index_t size_approx() const noexcept {
    return size_.load(std::memory_order::relaxed);
  }

 public:
  const allocator_t& get_allocator() const noexcept { return allocator_; }

 private:
  mutable std::mutex mutex_;
  std::condition_variable pop_available_cv_;
  std::condition_variable push_available_cv_;

  allocator_value_t* ring_buffer_ = nullptr;

  index_t push_to_ = 0;
  index_t pop_from_ = 0;
  alignas(
      std::hardware_destructive_interference_size) std::atomic<index_t> size_;

  [[no_unique_address]] allocator_t allocator_;
  [[no_unique_address]] const utils::conditionally_runtime<
      index_t, Capacity == utils::kDynamicExtent<index_t>, Capacity> capacity_;

 private:
  /**
   * @brief Preserves queue invariants. Assumes that no other thread is doing
   * any operations on the queue
   */
  void clear() noexcept {
    std::unique_lock<std::mutex> lock(mutex_);
    while (true) {
      if (pop_from_ == push_to_) {
        break;
      }

      std::destroy_at(&ring_buffer_[pop_from_].value());

      const index_t next_pop_from =
          (pop_from_ == *capacity_) ? 0 : pop_from_ + 1;
      pop_from_ = next_pop_from;
    }
    size_.store(0, std::memory_order::relaxed);
    push_available_cv_.notify_one();
  }
};

}  // namespace core::queues
