#pragma once

#include <atomic>
#include <coroutine>

#include <swiss/mixins.hpp>

namespace argo {

/**
 * @brief Shared thread-safe state that allows to safely control promise object
 */
class promise_ctrl final : public swiss::non_copyable,
                           public swiss::non_movable {
 public:
  constexpr explicit promise_ctrl(std::coroutine_handle<> handle) noexcept
      : handle_(handle), continuation_(std::noop_coroutine()) {}

 public:
  constexpr auto get_continuation() const noexcept -> std::coroutine_handle<> {
    return continuation_;
  }

  constexpr void set_continuation(
      std::coroutine_handle<> continuation) noexcept {
    continuation_ = continuation;
  }

  constexpr void add_ref() noexcept {
    // TODO:
    // SHARED(+1)
    use_count_.fetch_add(1);
  }

  constexpr void remove_ref() noexcept {
    // TODO:
    // (SUSPENDED | DONE) & SHARED(0) -> destroy
    // (SUSPENDED | DONE) & SHARED(_) ->
    if (use_count_.fetch_sub(1) == 1 && handle_) {
      handle_.destroy();
    }
  }

  constexpr auto done() const noexcept -> bool {
    // TODO:
    // SUSPENDED | RUNNING -> false
    // DONE -> true
    return handle_.done();
  }

  constexpr void resume() const {
    // TODO:
    // SUSPENDED -> {RUNNING}, resume
    // RUNNING | DONE -> throw
    handle_.resume();
  }

  constexpr auto use_count() const noexcept -> int { return use_count_.load(); }

 public:
  std::atomic<int> use_count_;
  std::coroutine_handle<> handle_;
  std::coroutine_handle<> continuation_;
};

}  // namespace argo
