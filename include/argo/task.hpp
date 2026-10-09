#pragma once

#include <stdexcept>

#include <swiss/mixins.hpp>

#include "promise.hpp"

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
  constexpr task() noexcept = delete;

  explicit constexpr task(promise_type & promise) noexcept
      : promise_(&promise) {}

  constexpr task(task && that) noexcept
      : promise_(std::exchange(that.promise_, nullptr)) {}

  constexpr auto operator=(task && that) noexcept -> task & {
    std::swap(promise_, that.promise_);
    return *this;
  }

  constexpr ~task() noexcept {
    if (promise_ != nullptr) {
      std::exchange(promise_, nullptr)->detach_return_object();
    }
  }

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
  // TODO: rework and extend this section
  constexpr auto suspend() -> suspended {
    return suspended(promise_->handle());
  }
  constexpr void resume() noexcept { promise_->resume(); }
  constexpr auto done() const noexcept -> bool { return promise_->done(); }
  constexpr auto get() -> decltype(auto) { return promise_->get(); }

 private:
  promise_type * promise_;
};

}  // namespace argo
