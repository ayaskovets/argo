#pragma once

#include <stdexcept>

#include <swiss/mixins.hpp>

#include "promise_type.hpp"

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
  using promise_type = promise_type<task>;

 public:
  explicit constexpr task(promise_ptr<promise_type> promise) noexcept
      : promise_(promise) {}

 public:
  /**
   * @note Rvalue-qualified operator to explicitly note that this operator
   * invokes the co_await'ed task immediately in the same thread and returns the
   * result
   */
  constexpr auto operator co_await() && -> decltype(auto) {
    if (promise_ == nullptr) {
      throw std::runtime_error("task::operator co_await(): no state");
    }
    return std::move(*promise_).operator co_await();
  }

 public:
  /**
   * @brief Resume a non-resumed coroutine that is not shared with a execution
   * channel, otherwise throw an exception
   * @warning Note that this happens without an implicit execution context so
   * the awaiter types that work with execution context will not work
   */
  constexpr void resume() { return promise_->resume(); }

  /**
   * @brief Check whether coroutine is finished
   */
  constexpr auto done() const noexcept -> bool { return promise_->done(); }

  /**
   * @brief Extract value from a finished coroutine. Throws an exception for an
   * unfinished coroutine
   * @returns A reference to a value of the task type or void
   */
  constexpr auto get() const -> decltype(auto) { return promise_->get(); }

 private:
  promise_ptr<promise_type> promise_;
};

}  // namespace argo
