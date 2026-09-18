#pragma once

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

  template <typename Caller>
  constexpr auto await_suspend(Caller && caller) -> decltype(auto) {
    return awaiter_.await_suspend(proxy_(std::forward<Caller>(caller)));
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
 * @section Utility awaiter constructors
 */
namespace argo::awaiters {

/**
 * @note This constructor supports awaiters with user-defined co_await operators
 */
template <typename Awaiter, typename Proxy>
constexpr auto proxy(Awaiter && awaiter, Proxy && proxy) noexcept
    -> decltype(auto) {
  if constexpr (requires { awaiter.operator co_await(); }) {
    return proxy_awaiter(std::forward<Awaiter>(awaiter).operator co_await(),
                         std::forward<Proxy>(proxy));
  } else if constexpr (requires { operator co_await(awaiter); }) {
    return proxy_awaiter(operator co_await(std::forward<Awaiter>(awaiter)),
                         std::forward<Proxy>(proxy));
  } else {
    return proxy_awaiter(std::forward<Awaiter>(awaiter),
                         std::forward<Proxy>(proxy));
  }
}

}  // namespace argo::awaiters