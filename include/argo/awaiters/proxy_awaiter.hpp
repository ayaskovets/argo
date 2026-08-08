#pragma once

#include <concepts>
#include <type_traits>

namespace argo::awaiters {

/**
 * @brief Utility awaiter that wraps the underlying awaiter type to restrict
 * access to the suspended coroutine handle by modifying it in a user-defined
 * way
 */
template <typename Awaiter, typename Proxy>
class proxy_awaiter final {
 public:
  explicit constexpr proxy_awaiter(Awaiter awaiter, Proxy proxy) noexcept
      : awaiter_(std::move(awaiter)), proxy_(std::move(proxy)) {}

 public:
  constexpr auto await_ready() noexcept(noexcept(awaiter_.await_ready()))
      -> decltype(auto) {
    return awaiter_.await_ready();
  }

  template <typename T>
  constexpr auto await_suspend(T && caller) noexcept(
      noexcept(awaiter_.await_suspend(proxy_(caller))) &&
      std::is_nothrow_invocable_v<Proxy, T &&>) -> decltype(auto) {
    static_assert(std::invocable<Proxy, T &&>,
                  "proxy_awaiter::await_suspend(): proxy object must be an "
                  "invocable accepting std::coroutine_handle");

    return awaiter_.await_suspend(proxy_(std::forward<T>(caller)));
  }

  constexpr auto await_resume() noexcept(noexcept(awaiter_.await_resume()))
      -> decltype(auto) {
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

template <typename Awaiter, typename Proxy>
constexpr auto proxy(Awaiter && awaiter, Proxy && proxy) noexcept
    -> decltype(auto) {
  auto underlying_awaiter = [](auto && awaiter) noexcept -> decltype(auto) {
    if constexpr (requires {
                    std::forward<Awaiter>(awaiter).operator co_await();
                  }) {
      return std::forward<Awaiter>(awaiter).operator co_await();
    } else if constexpr (requires {
                           operator co_await(std::forward<Awaiter>(awaiter));
                         }) {
      return operator co_await(std::forward<Awaiter>(awaiter));
    } else {
      return std::forward<Awaiter>(awaiter);
    }
  }(std::forward<Awaiter>(awaiter));

  return proxy_awaiter(std::move(underlying_awaiter),
                       std::forward<Proxy>(proxy));
}

}  // namespace argo::awaiters