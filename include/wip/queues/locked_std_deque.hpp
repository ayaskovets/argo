#pragma once

#include <swiss/conditionally_runtime.hpp>
#include <swiss/dynamic_extent.hpp>
#include <swiss/mixins.hpp>

#include <deque>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <type_traits>

namespace wip::queues {

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
          Index Capacity = swiss::kDynamicExtent<Index>,
          concepts::locked_std_deque_allocator<Value> Allocator =
              std::allocator<detail::locked_std_deque_entry<Value>>>
class locked_std_deque final : public swiss::non_copyable,
                               public swiss::non_movable {
 public:
  using value_type = Value;
  using index_type = Index;
  using allocator_type = Allocator;
  using allocator_value_type = typename Allocator::value_type;

 public:
  constexpr explicit locked_std_deque(
      const allocator_type& allocator = allocator_type()) noexcept(false)
    requires(Capacity != swiss::kDynamicExtent<index_type>)
      : deque_(allocator) {
    static_assert(Capacity > 0, "capacity must be greater than zero");
  }

  constexpr explicit locked_std_deque(
      index_type capacity,
      const allocator_type& allocator = allocator_type()) noexcept(false)
    requires(Capacity == swiss::kDynamicExtent<index_type>)
      : deque_(allocator), capacity_(capacity) {
    if (*capacity_ <= 0) [[unlikely]] {
      throw std::invalid_argument("capacity must be greater than zero");
    }
  }

  /**
   * @warning Destructor operations are not thread safe and it is assumed that
   * there is only one thread doing any operations on the queue at the moment
   * destruction occurs
   */
  constexpr ~locked_std_deque() noexcept = default;

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

  constexpr bool try_push(value_type value) {
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

  constexpr void push(value_type value) noexcept(false) {
    emplace(std::move(value));
  }

  constexpr bool try_pop(value_type& value) noexcept {
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

  constexpr value_type pop() noexcept {
    std::unique_lock lock(mutex_);
    pop_available_cv_.wait(lock, [this] { return !deque_.empty(); });

    value_type value(std::move(deque_.front().value()));
    deque_.pop_front();

    push_available_cv_.notify_one();
    return value;
  }

 public:
  constexpr index_type capacity() const noexcept { return *capacity_; }
  constexpr index_type size() const noexcept {
    std::lock_guard lock(mutex_);
    return deque_.size();
  }

 public:
  allocator_type get_allocator() const
      noexcept(std::is_nothrow_copy_constructible_v<allocator_type>) {
    return deque_.get_allocator();
  }

 private:
  mutable std::mutex mutex_;
  std::condition_variable pop_available_cv_;
  std::condition_variable push_available_cv_;
  std::deque<allocator_value_type, allocator_type> deque_;

  [[no_unique_address]] const swiss::conditionally_runtime<
      index_type, Capacity == swiss::kDynamicExtent<index_type>, Capacity>
      capacity_;
};

}  // namespace wip::queues
