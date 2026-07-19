#pragma once

#include <coroutine>

namespace argo {

/**
 * @brief There are a few pretty straightforward ways to extract the coroutine
 * handle from a task and do something unintended. This wrapper does at least
 * some hiding of the mutable part of the handle interface
 */
template <typename Promise = void>
class const_coroutine_handle;

template <>
class const_coroutine_handle<void> final : private std::coroutine_handle<> {
 public:
  // NOLINTNEXTLINE(hicpp-explicit-conversions,google-explicit-constructor)
  constexpr const_coroutine_handle(std::coroutine_handle<> handle) noexcept
      : std::coroutine_handle<>(handle) {}

 public:
  /**
   * @warning This method dangerous and is intended to be used only as at the
   * last point of usage of a handle where it's constness has to be dropped for
   * handle to be resumed e.g. in await_suspend
   */
  constexpr auto to_mutable_handle() const noexcept -> std::coroutine_handle<> {
    return *this;
  }

 public:
  using std::coroutine_handle<>::coroutine_handle;
  using std::coroutine_handle<>::operator bool;
  using std::coroutine_handle<>::done;
};

template <typename Promise>
class const_coroutine_handle final : private std::coroutine_handle<Promise> {
 public:
  // NOLINTNEXTLINE(hicpp-explicit-conversions,google-explicit-constructor)
  constexpr const_coroutine_handle(
      std::coroutine_handle<Promise> handle) noexcept
      : std::coroutine_handle<Promise>(handle) {}

  // NOLINTNEXTLINE(hicpp-explicit-conversions,google-explicit-constructor)
  constexpr operator const_coroutine_handle<>() const noexcept {
    return const_coroutine_handle<>(std::coroutine_handle<>(*this));
  }

 public:
  /**
   * @warning This method is intended to be used only as at the last point of
   * usage of a handle where it's constness has to be dropped for handle to be
   * resumed e.g. in await_suspend
   */
  constexpr auto to_mutable_handle() const noexcept
      -> std::coroutine_handle<Promise> {
    return *this;
  }

 public:
  using std::coroutine_handle<Promise>::coroutine_handle;
  using std::coroutine_handle<Promise>::operator bool;
  using std::coroutine_handle<Promise>::done;
};

}  // namespace argo
