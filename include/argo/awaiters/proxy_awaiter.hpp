#pragma once

#include <swiss/concepts.hpp>

#include "../awaiter.hpp"

namespace argo::awaiters {

/**
 * @brief Utility awaiter that wraps the underlying awaiter type to restrict
 * access to the suspended coroutine handle by modifying it in a user-defined
 * way
 */
template <concepts::awaiter Awaiter, typename Proxy>
class proxy_awaiter final {
 public:
  explicit constexpr proxy_awaiter(Awaiter awaiter, Proxy proxy) noexcept
      : awaiter_(std::move(awaiter)), proxy_(std::move(proxy)) {}

 public:
  constexpr auto await_ready() -> decltype(auto) {
    return awaiter_.await_ready();
  }

  constexpr auto await_suspend(auto && caller) -> decltype(auto) {
    return awaiter_.await_suspend(
        proxy_(std::forward<decltype(caller)>(caller)));
  }

  constexpr auto await_resume() -> decltype(auto) {
    return awaiter_.await_resume();
  }

 private:
  Awaiter awaiter_;
  [[no_unique_address]] Proxy proxy_;
};

}  // namespace argo::awaiters

/**
 * @section Awaiter constructors
 */
namespace argo::awaiters {

/**
 * @note This constructor supports awaiters with user-defined co_await operators
 */
template <typename Awaiter, typename Proxy>
constexpr auto proxy(Awaiter && awaiter, Proxy && proxy) noexcept
    -> decltype(auto) {
  return proxy_awaiter(swiss::co_await_result_v(std::forward<Awaiter>(awaiter)),
                       std::forward<Proxy>(proxy));
}

}  // namespace argo::awaiters