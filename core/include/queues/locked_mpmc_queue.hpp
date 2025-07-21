#pragma once

#include <atomic>
#include <condition_variable>
#include <deque>
#include <mutex>

#include "utils/conditionally_runtime.hpp"
#include "utils/constants.hpp"
#include "utils/mixins.hpp"

namespace core::queues {

/**
 * @brief Threadsafe queue for many producers and many consumers
 *
 * Implemented as a naively locked std::deque
 */
template <typename Value, std::unsigned_integral Index = std::size_t,
          Index Capacity = utils::kRuntimeDefinedCapacity<Index>,
          typename Allocator = std::allocator<Value>>
class locked_mpmc_queue final : utils::non_copyable, utils::non_movable {
 public:
  using value_t = Value;
  using index_t = Index;
  using allocator_t = Allocator;
  using entry_t = typename Allocator::value_type;

 private:
  static_assert(std::is_nothrow_destructible_v<entry_t>);
  static_assert(std::is_nothrow_move_constructible_v<entry_t>);
  static_assert(std::is_nothrow_move_assignable_v<entry_t>);
  static_assert(std::is_same_v<value_t, entry_t>);

 public:
  constexpr explicit locked_mpmc_queue(
      const allocator_t& allocator = allocator_t()) noexcept
    requires(Capacity != utils::kRuntimeDefinedCapacity<index_t> &&
             Capacity > 0)
      : queue_(allocator) {}

  constexpr explicit locked_mpmc_queue(
      index_t capacity, const allocator_t& allocator = allocator_t())
    requires(Capacity == utils::kRuntimeDefinedCapacity<index_t>)
      : queue_(allocator), capacity_(capacity) {
    if (*capacity_ <= 0) {
      throw std::invalid_argument("capacity must be greater than zero");
    }
  }

 public:
  bool try_push(value_t value) {
    if (size_.load(std::memory_order::relaxed) >= *capacity_) {
      return false;
    }
    std::unique_lock<std::mutex> lock(mutex_, std::defer_lock_t{});
    if (!lock.try_lock()) {
      return false;
    }
    if (queue_.size() >= *capacity_) {
      return false;
    }

    queue_.push_back(std::move(value));
    pop_available_cv_.notify_one();
    size_.fetch_add(1, std::memory_order::relaxed);
    return true;
  }

  void push(value_t value) {
    std::unique_lock<std::mutex> lock(mutex_);
    push_available_cv_.wait(lock,
                            [this] { return queue_.size() < *capacity_; });

    queue_.push_back(std::move(value));
    size_.fetch_add(1, std::memory_order::relaxed);
    pop_available_cv_.notify_one();
  }

  bool try_pop(value_t& value) {
    if (size_.load(std::memory_order::relaxed) == 0) {
      return false;
    }
    std::unique_lock<std::mutex> lock(mutex_, std::defer_lock_t{});
    if (!lock.try_lock()) {
      return false;
    }
    if (queue_.empty()) {
      return false;
    }

    value = std::move(queue_.front());
    queue_.pop_front();
    size_.fetch_sub(1, std::memory_order::relaxed);
    push_available_cv_.notify_one();
    return true;
  }

  value_t pop() {
    std::unique_lock<std::mutex> lock(mutex_);
    pop_available_cv_.wait(lock, [this] { return !queue_.empty(); });

    value_t front(
        std::move(queue_.front()));  ///< Correct iff noexcept(pop_front)

    queue_.pop_front();
    size_.fetch_sub(1, std::memory_order::relaxed);
    push_available_cv_.notify_one();
    return front;
  }

 public:
  constexpr index_t capacity() const noexcept { return *capacity_; }

 private:
  mutable std::mutex mutex_;
  std::condition_variable pop_available_cv_;
  std::condition_variable push_available_cv_;

  std::deque<value_t, allocator_t> queue_;
  std::atomic<index_t> size_;

  [[no_unique_address]] const utils::conditionally_runtime<
      index_t, Capacity == utils::kRuntimeDefinedCapacity<index_t>, Capacity>
      capacity_;
};

}  // namespace core::queues
