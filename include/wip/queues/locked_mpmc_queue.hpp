#pragma once

#include <swiss/conditionally_runtime.hpp>
#include <swiss/dynamic_extent.hpp>
#include <swiss/mixins.hpp>

#include <atomic>
#include <memory>
#include <mutex>
#include <new>
#include <stdexcept>

namespace wip::queues {

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
class alignas(Alignment) locked_mpmc_queue_entry final {
 public:
  constexpr Value& value() noexcept { return value_; }

 private:
  Value value_;
};

}  // namespace detail

/**
 * @brief Locked ring-buffer for many producers and many consumers
 *
 * Implemented as a ring buffer
 *
 * Allocated buffer always contains one additional element to allow for an
 * efficient implementation of a ring buffer indexing
 */
template <typename Value, std::unsigned_integral Index = std::size_t,
          Index Capacity = swiss::kDynamicExtent<Index>,
          concepts::locked_mpmc_queue_allocator<Value> Allocator =
              std::allocator<detail::locked_mpmc_queue_entry<Value>>>
class locked_mpmc_queue final : public swiss::non_copyable,
                                public swiss::non_movable {
 public:
  using value_type = Value;
  using index_type = Index;
  using allocator_type = Allocator;
  using allocator_value_type = typename Allocator::value_type;

 public:
  constexpr explicit locked_mpmc_queue(
      const allocator_type& allocator = allocator_type()) noexcept(false)
    requires(Capacity != swiss::kDynamicExtent<index_type>)
      : size_(0), allocator_(allocator) {
    static_assert(Capacity > 0, "capacity must be greater than zero");

    if (!(ring_buffer_ = allocator_.allocate(*capacity_ + 1))) [[unlikely]] {
      throw std::runtime_error("failed to allocate memory");
    }
  }

  constexpr explicit locked_mpmc_queue(
      index_type capacity,
      const allocator_type& allocator = allocator_type()) noexcept(false)
    requires(Capacity == swiss::kDynamicExtent<index_type>)
      : size_(0), allocator_(allocator), capacity_(capacity) {
    if (*capacity_ <= 0) [[unlikely]] {
      throw std::invalid_argument("capacity must be greater than zero");
    }

    if (!(ring_buffer_ = allocator_.allocate(*capacity_ + 1))) [[unlikely]] {
      throw std::runtime_error("failed to allocate memory");
    }
  }

  /**
   * @warning Destructor operations are not thread safe and it is assumed that
   * there is only one thread doing any operations on the queue at the moment
   * destruction occurs
   */
  constexpr ~locked_mpmc_queue() noexcept {
    while (push_to_ != pop_from_) {
      std::destroy_at(&ring_buffer_[pop_from_].value());

      const index_type next_pop_from =
          (pop_from_ == *capacity_) ? 0 : pop_from_ + 1;
      pop_from_ = next_pop_from;
    }

    allocator_.deallocate(ring_buffer_, *capacity_ + 1);
  }

 public:
  template <typename... Args>
  constexpr bool try_emplace(Args&&... args) noexcept
    requires(std::is_nothrow_constructible_v<Value, Args...>)
  {
    if (size_.load(std::memory_order::relaxed) >= *capacity_) {
      return false;
    }
    std::unique_lock lock(mutex_, std::defer_lock_t{});
    if (!lock.try_lock()) {
      return false;
    }
    const index_type next_push_to = (push_to_ == *capacity_) ? 0 : push_to_ + 1;
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

  constexpr bool try_push(value_type value) noexcept {
    return try_emplace(std::move(value));
  }

  template <typename... Args>
  constexpr void emplace(Args&&... args) noexcept
    requires(std::is_nothrow_constructible_v<Value, Args...>)
  {
    std::unique_lock lock(mutex_);
    push_available_cv_.wait(lock, [this] {
      const index_type next_push_to =
          (push_to_ == *capacity_) ? 0 : push_to_ + 1;
      return next_push_to != pop_from_;
    });

    std::construct_at(&ring_buffer_[push_to_].value(),
                      std::forward<Args>(args)...);

    const index_type next_push_to = (push_to_ == *capacity_) ? 0 : push_to_ + 1;
    push_to_ = next_push_to;
    size_.fetch_add(1, std::memory_order::relaxed);
    pop_available_cv_.notify_one();
  }

  constexpr void push(value_type value) noexcept { emplace(std::move(value)); }

  constexpr bool try_pop(value_type& value) noexcept {
    if (size_.load(std::memory_order::relaxed) == 0) {
      return false;
    }
    std::unique_lock lock(mutex_, std::defer_lock_t{});
    if (!lock.try_lock()) {
      return false;
    }
    if (pop_from_ == push_to_) {
      return false;
    }

    value = std::move(ring_buffer_[pop_from_].value());
    std::destroy_at(&ring_buffer_[pop_from_].value());

    const index_type next_pop_from =
        (pop_from_ == *capacity_) ? 0 : pop_from_ + 1;
    pop_from_ = next_pop_from;
    size_.fetch_sub(1, std::memory_order::relaxed);
    push_available_cv_.notify_one();
    return true;
  }

  constexpr value_type pop() noexcept {
    std::unique_lock lock(mutex_);
    pop_available_cv_.wait(lock, [this] { return pop_from_ != push_to_; });

    value_type value(std::move(ring_buffer_[pop_from_].value()));
    std::destroy_at(&ring_buffer_[pop_from_].value());

    const index_type next_pop_from =
        (pop_from_ == *capacity_) ? 0 : pop_from_ + 1;
    pop_from_ = next_pop_from;

    size_.fetch_sub(1, std::memory_order::relaxed);
    push_available_cv_.notify_one();
    return value;
  }

 public:
  constexpr index_type capacity() const noexcept { return *capacity_; }

  /**
   * @brief Approximate size
   */
  constexpr index_type size() const noexcept {
    return size_.load(std::memory_order::relaxed);
  }

 public:
  allocator_type get_allocator() const noexcept { return allocator_; }

 private:
  mutable std::mutex mutex_;
  std::condition_variable pop_available_cv_;
  std::condition_variable push_available_cv_;

  allocator_value_type* ring_buffer_ = nullptr;

  index_type push_to_ = 0;
  index_type pop_from_ = 0;
  alignas(std::hardware_destructive_interference_size)
      std::atomic<index_type> size_;

  [[no_unique_address]] allocator_type allocator_;
  [[no_unique_address]] const swiss::conditionally_runtime<
      index_type, Capacity == swiss::kDynamicExtent<index_type>, Capacity>
      capacity_;
};

}  // namespace wip::queues
