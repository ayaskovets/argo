#pragma once

#include <coroutine>
#include <cstddef>
#include <utility>

#include "promise_ctrl.hpp"

namespace argo {

/**
 * @brief Intrusive RAII ptr for notifying coroutine lifetime control about the
 * number of live instances of types that reference the coroutine state
 */
template <typename PromiseType>
class promise_ptr final {
 public:
  explicit constexpr promise_ptr(promise_ctrl & ctrl) noexcept : ctrl_(&ctrl) {
    ctrl_->add_ref();
  }

  constexpr promise_ptr(promise_ptr const & that) noexcept : ctrl_(that.ctrl_) {
    that.ctrl_->add_ref();
  }

  // NOLINTNEXTLINE(bugprone-unhandled-self-assignment)
  constexpr auto operator=(promise_ptr const & that) noexcept -> promise_ptr & {
    if (ctrl_ != that.ctrl_) {
      ctrl_->remove_ref();
      that.ctrl_->add_ref();
    }
    return *this;
  }

  constexpr promise_ptr(promise_ptr && that) noexcept
      : ctrl_(std::exchange(that.ctrl_, nullptr)) {}

  constexpr auto operator=(promise_ptr && that) noexcept -> promise_ptr & {
    std::swap(ctrl_, that.ctrl_);
    return *this;
  }

  constexpr ~promise_ptr() noexcept {
    if (ctrl_) {
      ctrl_->remove_ref();
    }
  }

 public:
  constexpr explicit operator bool() const noexcept { return ctrl_ != nullptr; }

  constexpr auto operator==(std::nullptr_t) const noexcept -> bool {
    return ctrl_ == nullptr;
  }

  constexpr auto operator*() const noexcept -> PromiseType & {
    return std::coroutine_handle<PromiseType>::from_address(
               ctrl_->handle_.address())
        .promise();
  }

  constexpr auto operator->() const noexcept -> PromiseType * {
    return &operator*();
  }

 private:
  promise_ctrl * ctrl_;
};

}  // namespace argo
