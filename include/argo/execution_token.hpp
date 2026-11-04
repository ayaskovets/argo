#pragma once

#include <cassert>
#include <utility>

#include <swiss/mixins.hpp>

namespace argo {
class promise_ctrl;
}  // namespace argo

/**
 * @note This section is required to mitigate the circular dependency between
 * promise control and execution token
 */
namespace argo::detail {
void promise_ctrl_unlock_resume(promise_ctrl &) noexcept;
void promise_ctrl_release_resume(promise_ctrl &) noexcept;
}  // namespace argo::detail

namespace argo {

/**
 * @brief Exclusive one-shot token for executing a coroutine. Only single object
 * of this type for a certain coroutine frame can be created
 */
// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions)
class execution_token final : public swiss::non_copyable {
 public:
  constexpr execution_token() noexcept : ctrl_(nullptr) {}

  explicit constexpr execution_token(promise_ctrl & ctrl) noexcept
      : ctrl_(&ctrl) {}

  constexpr execution_token(execution_token && that) noexcept
      : ctrl_(std::exchange(that.ctrl_, nullptr)) {}

  constexpr auto operator=(execution_token && that) noexcept
      -> execution_token & {
    std::swap(ctrl_, that.ctrl_);
    return *this;
  }

  constexpr ~execution_token() noexcept {
    if (ctrl_ != nullptr) {
      detail::promise_ctrl_release_resume(*ctrl_);
    }
  }

 private:
  constexpr void resume() && noexcept {
    assert(ctrl_ && "execution_token::resume(): no state");
    detail::promise_ctrl_unlock_resume(*std::exchange(ctrl_, nullptr));
  }

 private:
  promise_ctrl * ctrl_;

 private:
  /**
   * @brief For control to access itself in awaiter_suspend implementations
   */
  friend class promise_ctrl;

  /**
   * @brief For context to call resume
   */
  friend class execution_context;
};

}  // namespace argo
