#pragma once

#include <atomic>
#include <coroutine>
#include <exception>
#include <optional>
#include <stdexcept>
#include <utility>

#include <swiss/mixins.hpp>

#include "awaiters/continuation_awaiter.hpp"
#include "awaiters/proxy_awaiter.hpp"
#include "task_ref.hpp"

namespace argo {

// TODO: cancel for nested tasks

/**
 * @brief Coroutine return object
 */
template <typename T>
// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions)
class task : public swiss::non_copyable {
 public:
  using value_type = T;
  class promise_type;

 public:
  explicit constexpr task(std::coroutine_handle<promise_type> handle) noexcept
      : handle_(std::move(handle)) {}

  constexpr task(task && that) noexcept
      : handle_(std::exchange(that.handle_, nullptr)) {}

  constexpr auto operator=(task && that) noexcept -> task & {
    if (handle_) {
      /**
       * @note Assume destroy() does not throw
       */
      handle_.destroy();
    }

    handle_ = std::exchange(that.handle_, nullptr);
    return *this;
  }

  constexpr ~task() noexcept {
    if (handle_) {
      handle_.destroy();
    }
  }

 public:
  constexpr auto operator co_await() && noexcept -> decltype(auto) {
    struct task_awaiter final {
      // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
      // NOLINTBEGIN(cppcoreguidelines-avoid-const-or-ref-data-members)
      task & task;
      // NOLINTEND(cppcoreguidelines-avoid-const-or-ref-data-members)
      // NOLINTEND(misc-non-private-member-variables-in-classes)

     public:
      constexpr auto await_ready() const noexcept -> bool {
        return task.done();
      }

      constexpr auto await_suspend(std::coroutine_handle<> caller)
          const noexcept -> std::coroutine_handle<> {
        task.handle_.promise().set_continuation(caller);
        return task.handle_;
      }

      constexpr auto await_resume() -> decltype(auto) {
        return std::move(task).get();
      }
    };
    return task_awaiter{.task = *this};
  }

 public:
  explicit constexpr operator bool() const noexcept {
    return static_cast<bool>(handle_);
  }

  constexpr void operator()() { resume(); }

 public:
  constexpr auto done() const noexcept -> bool {
    return !handle_ || handle_.done();
  };

  constexpr void resume() {
    if (!handle_) [[unlikely]] {
      throw std::runtime_error("task::resume(): no state");
    }
    if (handle_.done()) [[unlikely]] {
      throw std::runtime_error("task::resume(): already done");
    }

    handle_.resume();
  }

  constexpr auto get() && -> decltype(auto) {
    if (!handle_) [[unlikely]] {
      throw std::runtime_error("task::get(): no state");
    }
    if (!handle_.done()) {
      throw std::runtime_error("task::get(): not done");
    }

    return std::move(handle_.promise()).get();
  }

 private:
  std::coroutine_handle<promise_type> handle_;

 private:
  /**
   * @note For passing coroutine handle to task ref in constructor
   */
  friend class task_ref;
};

/**
 * @brief Shared parts of promise type with void and non-void return type
 */
class base_promise_type : public swiss::non_copyable,
                          public swiss::non_movable {
 public:
  constexpr auto initial_suspend() const noexcept -> std::suspend_always {
    return {};
  }

  constexpr auto final_suspend() noexcept -> awaiters::continuation_awaiter {
    /**
     * @note Check whether there is a continuation for this task or assign a
     * sentinel value indicating that the task is finished without subsequent
     * symmetric transfer
     */
    if (void * expected = nullptr; !continuation_.compare_exchange_strong(
            expected, this, std::memory_order::acquire,
            std::memory_order::relaxed)) {
      return awaiters::continuation_awaiter(
          std::coroutine_handle<>::from_address(expected));
    }
    return awaiters::continuation_awaiter(nullptr);
  }

  constexpr void unhandled_exception() noexcept {
    exception_ = std::current_exception();
  }

 public:
  template <typename U>
  constexpr auto await_transform(task<U> & task) const noexcept
      -> decltype(auto) {
    return task;
  }

  template <typename U>
  constexpr auto await_transform(task<U> && task) const noexcept
      -> decltype(auto) {
    return std::move(task);
  }

  template <concepts::awaiter Awaiter>
  constexpr auto await_transform(Awaiter && awaiter) const noexcept
      -> decltype(auto) {
    return awaiters::proxy(
        std::forward<Awaiter>(awaiter),
        [](std::coroutine_handle<> caller) noexcept -> decltype(auto) {
          return task_ref(caller);
        });
  }

 public:
  constexpr void set_continuation(
      std::coroutine_handle<> continuation) noexcept {
    continuation_.store(continuation.address(), std::memory_order::release);
  }

 protected:
  // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
  std::exception_ptr exception_     = nullptr;
  std::atomic<void *> continuation_ = nullptr;
  // NOLINTEND(misc-non-private-member-variables-in-classes)
};

/**
 * @brief Promise type for a non-void task
 */
template <typename T>
class task<T>::promise_type final : public base_promise_type {
 public:
  constexpr auto get_return_object() noexcept -> task<T> {
    return task<T>(std::coroutine_handle<promise_type>::from_promise(*this));
  }

  constexpr void return_value(T && value) noexcept {
    value_.emplace(std::move(value));
  }

 public:
  constexpr auto get() && -> T & {
    if (exception_) {
      std::rethrow_exception(exception_);
    }

    return *value_;
  }

 private:
  std::optional<T> value_;
};

/**
 * @brief Promise type for a void task
 */
template <>
class task<void>::promise_type final : public base_promise_type {
 public:
  constexpr auto get_return_object() noexcept -> task<void> {
    return task<void>(std::coroutine_handle<promise_type>::from_promise(*this));
  }

  constexpr void return_void() const noexcept {}

 public:
  constexpr void get() && {
    if (exception_) {
      std::rethrow_exception(exception_);
    }
  }
};

}  // namespace argo
