#pragma once

#include <coroutine>
#include <utility>

#include "promise_ctrl.hpp"

namespace argo {
template <typename ReturnObject>
class promise_frame;
}  // namespace argo

namespace argo {

/**
 * @brief Intrusive RAII ptr for notifying coroutine lifetime control about the
 * number of live instances of types that reference the coroutine state
 * @note Requires the specific promise type to for functions that access the
 * promise frame
 */
template <typename PromiseType = void>
class promise_ptr final {
 public:
  constexpr promise_ptr() noexcept : ctrl_(nullptr) {}

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

  constexpr auto operator*() const noexcept -> decltype(auto)
    requires(!std::same_as<PromiseType, void>)
  {
    return std::coroutine_handle<PromiseType>::from_address(
               ctrl_->handle_.address())
        .promise();
  }

  constexpr auto operator->() const noexcept -> PromiseType *
    requires(!std::same_as<PromiseType, void>)
  {
    return &operator*();
  }

 private:
  promise_ctrl * ctrl_;

 private:
  /**
   * @note For execution token to access control object and call resume
   */
  friend class execution_token;

  /**
   * @note For promise frame to access the control pointer
   */
  template <typename ReturnObject>
  friend class promise_frame;
};

}  // namespace argo
