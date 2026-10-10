#pragma once

#include <coroutine>
#include <exception>
#include <optional>
#include <stdexcept>

#include <swiss/mixins.hpp>

#include "awaiter.hpp"

namespace argo {

/**
 * @brief Mandatory return value interface for a non-void-returning coroutine
 */
template <typename T>
class promise_type_return_value {
 public:
  template <typename U>
  constexpr void return_value(U && value) noexcept {
    value_.emplace(std::forward<U>(value));
  }

  constexpr void unhandled_exception() noexcept {
    exception_ = std::current_exception();
  }

 protected:
  constexpr auto get() -> T & {
    if (exception_) {
      std::rethrow_exception(exception_);
    }
    if (!value_.has_value()) {
      throw std::runtime_error("promise_type_return_value::get(): not done");
    }
    return *value_;
  }

 private:
  std::optional<T> value_;
  std::exception_ptr exception_;
};

/**
 * @brief Mandatory return value interface for a void-returning coroutine
 */
template <>
class promise_type_return_value<void> {
 public:
  constexpr void return_void() const noexcept {}

  constexpr void unhandled_exception() noexcept {
    exception_ = std::current_exception();
  }

 protected:
  constexpr void get() const {
    if (exception_) {
      std::rethrow_exception(exception_);
    }
  }

 private:
  std::exception_ptr exception_;
};

/**
 * @brief Shared thread-safe promise state. Separated into a dedicated type to
 * localize promise mutations
 */
class promise_type_shared_state final {
 public:
  constexpr promise_type_shared_state() noexcept
      : continuation_(std::noop_coroutine()) {}

 public:
  constexpr auto get_continuation() const noexcept -> std::coroutine_handle<> {
    return continuation_;
  }

  constexpr void set_continuation(
      std::coroutine_handle<> continuation) noexcept {
    continuation_ = continuation;
  }

  // TODO: impl

 private:
  std::coroutine_handle<> continuation_;
};

/**
 * @brief Promise type with configurable return object type and a state machine
 * for lifetime management
 */
template <typename ReturnObject>
class promise_type final
    : public promise_type_return_value<typename ReturnObject::value_type>,
      public swiss::non_copyable,
      public swiss::non_movable {
 public:
  constexpr auto get_return_object() noexcept -> ReturnObject {
    return ReturnObject(*this);
  }

 public:
  /**
   * @brief Hide the raw coroutine handle from the user-facing awaiter types and
   * replace it with a fixed awaiter context with a curated set of possible
   * operations on the coroutine
   * @note Supports awaiters that do not care about the suspended task where
   * await_suspend takes no arguments
   */
  template <concepts::awaiter Awaiter>
  constexpr auto await_transform(Awaiter && awaiter) const noexcept
      -> decltype(auto) {
    class proxy_awaiter final {
     public:
      explicit constexpr proxy_awaiter(
          swiss::co_await_result_t<Awaiter> awaiter) noexcept
          : awaiter_(std::move(awaiter)) {}

     public:
      constexpr auto await_ready() const noexcept -> bool {
        return awaiter_.await_ready();
      }

      constexpr auto await_suspend(std::coroutine_handle<> handle)
          -> decltype(auto) {
        if constexpr (requires { awaiter_.await_suspend(); }) {
          return awaiter_.await_suspend();
        } else {
          return awaiter_.await_suspend(execution_token(handle));
        }
      }

      constexpr auto await_resume() -> decltype(auto) {
        return awaiter_.await_resume();
      }

     private:
      swiss::co_await_result_t<Awaiter> awaiter_;
    };

    return proxy_awaiter(
        swiss::co_await_result_v(std::forward<Awaiter>(awaiter)));
  }

 public:
  /**
   * @brief Lazy execution by default
   */
  constexpr auto initial_suspend() const noexcept -> decltype(auto) {
    return std::suspend_always();
  }

  /**
   * @brief Suspend the task and do a symmetric transfer if there is a single
   * awaiting parent coroutine
   */
  constexpr auto final_suspend() const noexcept -> decltype(auto) {
    class continuation_awaiter final {
     public:
      explicit constexpr continuation_awaiter(
          promise_type const & promise) noexcept
          : promise_(&promise) {}

     public:
      constexpr auto await_ready() const noexcept -> bool { return false; }

      constexpr auto await_suspend(std::coroutine_handle<>) const noexcept
          -> std::coroutine_handle<> {
        return promise_->shared_state_.get_continuation();
      }

      constexpr void await_resume() const noexcept {}

     private:
      promise_type const * promise_;
    };

    return continuation_awaiter(*this);
  }

  /**
   * @brief Synchronously resume this coroutine via symmetric transfer from the
   * suspended task and return control to the parent in final_suspend()
   */
  constexpr auto operator co_await() && noexcept -> decltype(auto) {
    class scope_awaiter final {
     public:
      explicit constexpr scope_awaiter(promise_type & promise) noexcept
          : promise_(&promise) {}

     public:
      constexpr auto await_ready() const noexcept -> bool {
        return promise_->handle().done();
      }

      constexpr auto await_suspend(execution_token token) const noexcept
          -> std::coroutine_handle<> {
        promise_->shared_state_.set_continuation(token.handle_);
        return promise_->handle();
      }

      constexpr auto await_resume() -> decltype(auto) {
        return promise_->get();
      }

     private:
      promise_type * promise_;
    };

    return scope_awaiter(*this);
  }

 public:
  // TODO: rework and extend this section
  constexpr auto handle() noexcept -> std::coroutine_handle<promise_type> {
    return std::coroutine_handle<promise_type>::from_promise(*this);
  }
  constexpr void detach_return_object() { handle().destroy(); }
  constexpr auto done() -> bool { return handle().done(); }
  constexpr void resume() { handle().resume(); }
  using promise_type_return_value<typename ReturnObject::value_type>::get;

 private:
  promise_type_shared_state shared_state_;
};

}  // namespace argo
