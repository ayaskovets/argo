#pragma once

#include <concepts>
#include <coroutine>
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

  template <typename Promise>
  constexpr auto await_suspend(std::coroutine_handle<Promise> parent) noexcept(
      noexcept(awaiter_.await_suspend(parent)) &&
      std::is_nothrow_invocable_v<Proxy, std::coroutine_handle<Promise>>)
      -> decltype(auto) {
    static_assert(std::invocable<Proxy, std::coroutine_handle<Promise>>,
                  "proxy_awaiter::await_suspend(): proxy object must be an "
                  "invocable accepting std::coroutine_handle");

    return awaiter_.await_suspend(proxy_(parent));
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
    -> proxy_awaiter<Awaiter, Proxy> {
  return proxy_awaiter(std::forward<Awaiter>(awaiter),
                       std::forward<Proxy>(proxy));
}

}  // namespace argo::awaiters