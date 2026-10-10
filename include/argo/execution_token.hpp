#pragma once

#include <swiss/mixins.hpp>

#include "promise_ptr.hpp"

namespace argo {
template <typename ReturnObject>
class promise_frame;
}  // namespace argo

namespace argo {

/**
 * @brief Essentially a mutex for the resume() call only instead of blocking
 * throws an exception as more than one instance of this object for a
 * certain coroutine frame is counted as in invalid state caused by program
 * error
 */
class execution_token final : public swiss::non_copyable {
 public:
  explicit constexpr execution_token(promise_ptr<> promise = nullptr) noexcept
      : promise_(std::move(promise)) {}

 public:
  constexpr void resume() const { promise_.resume({}); }

 private:
  promise_ptr<> promise_;

 private:
  /**
   * @note For promise frame to access the control pointer
   */
  template <typename ReturnObject>
  friend class promise_frame;
};

}  // namespace argo
