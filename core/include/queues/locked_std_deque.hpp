#pragma once

#include <deque>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <type_traits>

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
  { entry.value() } -> std::same_as<Value&>;

  requires noexcept(entry.value());

  requires std::is_nothrow_destructible_v<Value>;
  requires std::is_nothrow_move_constructible_v<Value>;
  requires std::is_nothrow_move_assignable_v<Value>;
};

template <typename T, typename Value>
concept locked_std_deque_allocator = requires {
  requires locked_std_deque_entry<typename T::value_type, Value>;
};

}  // namespace concepts

namespace detail {

/**
 * @brief Default queue entry only for internal usage
 */
template <typename Value, std::size_t Alignment = alignof(Value)>
class alignas(Alignment) locked_std_deque_entry final {
 public:
  constexpr explicit locked_std_deque_entry(Value&& value) noexcept
      : value_(std::move(value)) {}

 public:
  constexpr Value& value() noexcept { return value_; }

 private:
  Value value_;
};

}  // namespace detail

/**
 * @brief Locked std::queue. This class is intended to represent a naive
 * baseline implementation of a MPMC queue
 */
template <typename Value, std::unsigned_integral Index = std::size_t,
          Index Capacity = utils::kDynamicExtent<Index>,
          concepts::locked_std_deque_allocator<Value> Allocator =
              std::allocator<detail::locked_std_deque_entry<Value>>>
class locked_std_deque : public utils::non_copyable, public utils::non_movable {
 public:
  using value_t = Value;
  using index_t = Index;
  using allocator_t = Allocator;
  using allocator_value_t = typename Allocator::value_type;

 public:
  constexpr explicit locked_std_deque(
      const allocator_t& allocator = allocator_t()) noexcept(false)
    requires(Capacity != utils::kDynamicExtent<index_t> && Capacity > 0)
      : deque_(allocator) {}

  constexpr explicit locked_std_deque(
      index_t capacity,
      const allocator_t& allocator = allocator_t()) noexcept(false)
    requires(Capacity == utils::kDynamicExtent<index_t>)
      : deque_(allocator), capacity_(capacity) {
    if (*capacity_ <= 0) [[unlikely]] {
      throw std::invalid_argument("capacity must be greater than zero");
    }
  }

  constexpr ~locked_std_deque() noexcept { clear(); }

 public:
  template <typename... Args>
  constexpr bool try_emplace(Args&&... args) noexcept(false)
    requires(std::is_constructible_v<Value, Args...>)
  {
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
  constexpr void emplace(Args&&... args) noexcept(false)
    requires(std::is_constructible_v<Value, Args...>)
  {
    std::unique_lock lock(mutex_);
    push_available_cv_.wait(lock,
                            [this] { return deque_.size() != *capacity_; });

    deque_.emplace_back(std::forward<Args>(args)...);

    pop_available_cv_.notify_one();
  }

  constexpr void push(value_t value) noexcept(false) {
    emplace(std::move(value));
  }

  constexpr bool try_pop(value_t& value) noexcept {
    std::unique_lock lock(mutex_, std::defer_lock_t{});
    if (!lock.try_lock()) {
      return false;
    }

    if (deque_.empty()) {
      return false;
    }

    value = std::move(deque_.front().value());
    deque_.pop_front();

    push_available_cv_.notify_one();
    return true;
  }

  constexpr value_t pop() noexcept {
    std::unique_lock lock(mutex_);
    pop_available_cv_.wait(lock, [this] { return !deque_.empty(); });

    value_t value(std::move(deque_.front().value()));
    deque_.pop_front();

    push_available_cv_.notify_one();
    return value;
  }

 public:
  constexpr index_t capacity() const noexcept { return *capacity_; }
  constexpr index_t size() const noexcept {
    std::lock_guard lock(mutex_);
    return deque_.size();
  }

 public:
  allocator_t get_allocator() const
      noexcept(std::is_nothrow_copy_constructible_v<allocator_t>) {
    return deque_.get_allocator();
  }

 private:
  mutable std::mutex mutex_;
  std::condition_variable pop_available_cv_;
  std::condition_variable push_available_cv_;
  std::deque<allocator_value_t, allocator_t> deque_;

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
