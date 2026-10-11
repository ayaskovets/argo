#pragma once

#include <stdexcept>

#include <swiss/mixins.hpp>

#include "execution_token.hpp"
#include "promise_frame.hpp"
#include "task_future.hpp"

namespace argo {

/**
 * @brief Default value- or void- returning task object for a coroutine.
 * Interfaces with the coroutine state through the promise object
 */
template <typename T>
// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions)
class task final : public swiss::non_copyable {
 public:
  using value_type   = T;
  using promise_type = promise_frame<task>;

 public:
  explicit constexpr task(promise_ptr<promise_type> ptr) noexcept
      : ptr_(std::move(ptr)) {}

 public:
  /**
   * @note Rvalue-qualified operator to explicitly note that this operator
   * invokes the co_await'ed task immediately in the same thread and returns the
   * result
   */
  constexpr auto operator co_await() && -> decltype(auto) {
    if (ptr_ == nullptr) [[unlikely]] {
      throw std::runtime_error("task::operator co_await(): no state");
    }
    return std::move(*ptr_).operator co_await();
  }

 public:
  /**
   * @brief Lock other calls to resume and this function. Create an object
   * that grants exclusive access resumption until it gets destructed
   */
  constexpr auto get_execution_token() -> execution_token {
    return ptr_->get_execution_token();
  }

  /**
   * @brief Get future for the task object. Does not run or schedule the task
   */
  constexpr auto get_future() const noexcept -> task_future<T> {
    return task_future<T>(ptr_);
  }

  /**
   * @brief Get future for the task object. Destroy the task object
   */
  constexpr auto get_future() && noexcept -> task_future<T> {
    return task_future<T>(std::move(ptr_));
  }

 public:
  /**
   * @brief Resume a non-resumed coroutine that does not have a live
   * execution token, otherwise throw an exception
   * @warning Synchronous interface. Not intended for general use within the
   * engine
   */
  constexpr void resume() { return ptr_->resume(); }

  /**
   * @brief Check whether coroutine is finished
   * @warning Synchronous interface. Not intended for general use within the
   * engine
   */
  constexpr auto done() const noexcept -> bool { return ptr_->done(); }

  /**
   * @brief Extract value from a finished coroutine. Throws an exception for an
   * unfinished coroutine
   * @returns A reference to a value of the task type or void
   * @warning Synchronous interface. Not intended for general use within the
   * engine
   */
  constexpr auto get() -> decltype(auto) { return ptr_->get(); }

 private:
  promise_ptr<promise_type> ptr_;
};

}  // namespace argo
