#pragma once

#include <atomic>
#include <cassert>
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
    class task_awaiter final {
     public:
      explicit constexpr task_awaiter(task<T> && task) noexcept
          : task_(std::move(task)) {}

     public:
      constexpr auto await_ready() const noexcept -> bool {
        return task_.done();
      }

      constexpr auto await_suspend(std::coroutine_handle<> caller)
          const noexcept -> std::coroutine_handle<> {
        assert(task_.handle_.promise().compare_exchange_continuation(caller) &&
               "task_awaiter::await_suspend(): failed to set continuation");
        return task_.handle_;
      }

      constexpr auto await_resume() -> decltype(auto) {
        return std::move(task_).get();
      }

     private:
      task<T> task_;
    };
    return task_awaiter(std::move(*this));
  }

 public:
  explicit constexpr operator bool() const noexcept {
    return static_cast<bool>(handle_);
  }

  constexpr void operator()() { resume(); }

 public:
  constexpr auto done() const noexcept -> bool {
    return !handle_ || handle_.done();
  }

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
template <typename T, typename CRTP>
// NOLINTNEXTLINE(bugprone-crtp-constructor-accessibility)
class base_promise_type : public swiss::non_copyable,
                          public swiss::non_movable {
 public:
  constexpr auto get_return_object() noexcept -> task<T> {
    return task<T>(
        std::coroutine_handle<CRTP>::from_promise(static_cast<CRTP &>(*this)));
  }

  constexpr auto initial_suspend() const noexcept -> std::suspend_always {
    return {};
  }

  /**
   * @brief Check whether there is a continuation for this task or assign a
   * sentinel value indicating that the task is finished
   */
  constexpr auto final_suspend() noexcept -> awaiters::continuation_awaiter {
    if (std::coroutine_handle<> continuation = continuation_sentinel();
        !compare_exchange_continuation(continuation)) {
      return awaiters::continuation(continuation);
    }
    return awaiters::continuation(nullptr);
  }

  constexpr void unhandled_exception() noexcept {
    exception_ = std::current_exception();
  }

 public:
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
  constexpr auto done_without_continuation() noexcept -> bool {
    return continuation_.load(std::memory_order::relaxed) ==
           continuation_sentinel();
  }

  /**
   * @return True if this is the first call of this method on a certain promise
   * object and if
   */
  constexpr auto compare_exchange_continuation(
      std::coroutine_handle<> & continuation) noexcept -> bool {
    std::coroutine_handle<> expected = nullptr;
    if (continuation_.compare_exchange_strong(expected, continuation,
                                              std::memory_order::acquire,
                                              std::memory_order::relaxed)) {
      return true;
    }

    continuation = expected;
    return false;
  }

  constexpr void get() && {
    if (exception_) {
      std::rethrow_exception(exception_);
    }
  }

 private:
  std::exception_ptr exception_{nullptr};
  std::atomic<std::coroutine_handle<>> continuation_{nullptr};

 private:
  /**
   * @brief Sentinel value for continuation indicating that promise type is
   * already in the finished state without any continuation task
   */
  constexpr auto continuation_sentinel() noexcept -> std::coroutine_handle<> {
    return std::coroutine_handle<CRTP>::from_promise(
        static_cast<CRTP &>(*this));
  }
};

/**
 * @brief Promise type for a non-void task
 */
template <typename T>
class task<T>::promise_type : public base_promise_type<T, promise_type> {
 public:
  constexpr void return_value(T && value) noexcept {
    value_.emplace(std::move(value));
  }

 public:
  constexpr auto get() && -> T & {
    std::move(*this).base_promise_type<T, promise_type>::get();
    return *value_;
  }

 private:
  std::optional<T> value_;
};

/**
 * @brief Promise type for a void task
 */
template <>
class task<void>::promise_type : public base_promise_type<void, promise_type> {
 public:
  constexpr void return_void() const noexcept {}
};

}  // namespace argo
