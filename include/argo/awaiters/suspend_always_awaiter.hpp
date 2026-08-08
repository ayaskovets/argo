#pragma once

namespace argo::awaiters {

/**
 * @brief Utility awaiter with same semantics as std::suspend_always
 * @warning This awaiter does not re-schedule the suspended task and is not
 * intended to be used in an executor context
 */
class suspend_always_awaiter final {
 public:
  constexpr auto await_ready() const noexcept -> bool { return false; }
  constexpr void await_suspend(auto &&) const noexcept {}
  constexpr void await_resume() const noexcept {}
};

}  // namespace argo::awaiters

/**
 * @section Utility awaiter constructors
 */
namespace argo::awaiters {

constexpr auto suspend_always() noexcept -> suspend_always_awaiter {
  return {};
}

}  // namespace argo::awaiters
