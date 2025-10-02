#pragma once

#include <deque>
#include <memory>
#include <mutex>
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
concept locked_std_deque_entry = requires(T entry) {
  // TODO: impl
  requires true;
};

template <typename T, typename Value>
concept locked_std_deque_allocator = requires {
  requires locked_std_deque_entry<typename T::value_type, Value>;
};

}  // namespace concepts

/**
 * @brief Locked std::queue. This class is intended to represent a naive
 * baseline implementation of a MPMC queue
 */
template <typename Value, std::unsigned_integral Index = std::size_t,
          Index Capacity = utils::kDynamicExtent<Index>,
          concepts::locked_std_deque_allocator<Value> Allocator =
              std::allocator<Value>>
class locked_std_deque final : utils::non_copyable, utils::non_movable {
 public:
  using value_t = Value;
  using index_t = Index;
  using allocator_t = Allocator;
  using allocator_value_t = typename Allocator::value_type;

 public:
  constexpr explicit locked_std_deque(
      const allocator_t& allocator = allocator_t())
    requires(Capacity != utils::kDynamicExtent<index_t> && Capacity > 0)
      : deque_(allocator) {}

  constexpr explicit locked_std_deque(
      index_t capacity, const allocator_t& allocator = allocator_t())
    requires(Capacity == utils::kDynamicExtent<index_t>)
      : deque_(allocator), capacity_(capacity) {
    if (*capacity_ <= 0) [[unlikely]] {
      throw std::invalid_argument("capacity must be greater than zero");
    }
  }

  constexpr ~locked_std_deque() noexcept { clear(); }

 public:
  template <typename... Args>
  constexpr bool try_emplace(Args&&... args) {
    std::unique_lock lock(mutex_, std::defer_lock_t{});
    if (!lock.try_lock()) {
      return false;
    }

    if (deque_.size() == *capacity_) {
      return false;
    }

    deque_.emplace_back(std::forward<Args>(args)...);

    pop_available_cv_.notify_one();
    return true;
  }

  constexpr bool try_push(value_t value) {
    return try_emplace(std::move(value));
  }

  template <typename... Args>
  constexpr void emplace(Args&&... args) {
    std::unique_lock lock(mutex_);
    push_available_cv_.wait(lock,
                            [this] { return deque_.size() != *capacity_; });

    deque_.emplace_back(std::forward<Args>(args)...);

    pop_available_cv_.notify_one();
  }

  constexpr void push(value_t value) { emplace(std::move(value)); }

  constexpr bool try_pop(value_t& value) noexcept {
    std::unique_lock lock(mutex_, std::defer_lock_t{});
    if (!lock.try_lock()) {
      return false;
    }

    if (deque_.empty()) {
      return false;
    }

    value = std::move(deque_.front());
    deque_.pop_front();

    push_available_cv_.notify_one();
    return true;
  }

  constexpr value_t pop() noexcept {
    std::unique_lock lock(mutex_);
    pop_available_cv_.wait(lock, [this] { return !deque_.empty(); });

    value_t value(std::move(deque_.front()));
    deque_.pop_front();

    push_available_cv_.notify_one();
    return value;
  }

 public:
  constexpr index_t size() const noexcept {
    std::lock_guard lock(mutex_);
    return deque_.size();
  }
  constexpr index_t capacity() const noexcept { return *capacity_; }

 private:
  mutable std::mutex mutex_;
  std::condition_variable pop_available_cv_;
  std::condition_variable push_available_cv_;
  std::deque<allocator_value_t, allocator_t> deque_;

  [[no_unique_address]] allocator_t allocator_;
  [[no_unique_address]] const utils::conditionally_runtime<
      index_t, Capacity == utils::kDynamicExtent<index_t>, Capacity> capacity_;

 private:
  /**
   * @brief Preserves queue invariants. Assumes that no other thread is doing
   * any operations on the queue
   */
  void clear() noexcept {
    std::unique_lock lock(mutex_);
    deque_.clear();

    push_available_cv_.notify_all();
  }
};

}  // namespace core::queues
