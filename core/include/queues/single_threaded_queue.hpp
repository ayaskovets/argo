#pragma once

#include <atomic>
#include <optional>
#include <type_traits>

#include "utils/conditionally_runtime.hpp"
#include "utils/constants.hpp"
#include "utils/mixins.hpp"

namespace core::queues {

namespace detail {

template <typename T, typename Value>
concept single_threaded_queue_entry = requires(T entry) {
  { entry.value() } -> std::same_as<Value&>;
};

}  // namespace detail

template <typename Value>
class single_threaded_queue_entry final {
 private:
  Value value_;

 public:
  constexpr Value& value() { return value_; }
};

// NOTE: this queue is not thread safe and is expected to be used in only the
// single-threaded context, otherwise this is the same as waitfree_spsc_queue
template <typename Value, std::unsigned_integral Index = std::size_t,
          Index Capacity = utils::kRuntimeDefinedCapacity<Index>,
          typename Allocator =
              std::allocator<single_threaded_queue_entry<Value>>>
class single_threaded_queue final : utils::non_copyable, utils::non_movable {
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
  static_assert(detail::single_threaded_queue_entry<entry_t, value_t>);

 public:
  constexpr explicit single_threaded_queue(
      const allocator_t& allocator = allocator_t())
    requires(Capacity != utils::kRuntimeDefinedCapacity<index_t> &&
             Capacity > 0)
      : allocator_(allocator) {
    if (!(ring_buffer_ = allocator_.allocate(*capacity_ + 1))) [[unlikely]] {
      throw std::runtime_error("failed to allocate memory");
    }
  }

  constexpr explicit single_threaded_queue(
      index_t capacity, const allocator_t& allocator = allocator_t())
    requires(Capacity == utils::kRuntimeDefinedCapacity<index_t>)
      : allocator_(allocator), capacity_(capacity) {
    if (*capacity_ <= 0) [[unlikely]] {
      throw std::invalid_argument("capacity must be greater than zero");
    }

    if (!(ring_buffer_ = allocator_.allocate(*capacity_ + 1))) [[unlikely]] {
      throw std::runtime_error("failed to allocate memory");
    }
  }

  constexpr ~single_threaded_queue() noexcept {
    value_t value;
    while (try_pop(value));
    allocator_.deallocate(ring_buffer_, *capacity_ + 1);
  }

 public:
  constexpr bool try_push(value_t value) noexcept {
    const index_t next_push_to = (push_to_ == *capacity_) ? 0 : push_to_ + 1;
    if (next_push_to == pop_from_) {
      return false;
    }

    std::construct_at(&ring_buffer_[push_to_].value(), std::move(value));

    push_to_ = next_push_to;
    return true;
  }

  constexpr bool try_pop(value_t& value) noexcept {
    if (pop_from_ == push_to_) {
      return false;
    }

    value = std::move(ring_buffer_[pop_from_].value());
    std::destroy_at(&ring_buffer_[pop_from_].value());

    const index_t next_pop_from = (pop_from_ == *capacity_) ? 0 : pop_from_ + 1;
    pop_from_ = next_pop_from;
    return true;
  }

 public:
  constexpr bool empty() const noexcept { return push_to_ == pop_from_; }
  constexpr index_t size() const noexcept {
    return pop_from_ <= push_to_ ? push_to_ - pop_from_
                                 : pop_from_ - push_to_ - 1;
  }
  constexpr index_t capacity() const noexcept { return *capacity_; }

 private:
  entry_t* ring_buffer_ = nullptr;

  index_t push_to_ = 0;
  index_t pop_from_ = 0;

  [[no_unique_address]] allocator_t allocator_;
  [[no_unique_address]] const utils::conditionally_runtime<
      index_t, Capacity == utils::kRuntimeDefinedCapacity<index_t>, Capacity>
      capacity_;
};

}  // namespace core::queues
