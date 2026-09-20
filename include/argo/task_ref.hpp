#pragma once

#include <coroutine>
#include <stdexcept>

namespace argo {

template <typename T>
class task;

/**
 * @brief Non-owning trivially copyable task reference with reduced interface
 * intended for passing type-erase tasks to a scheduler or an executor without
 * passing task ownership
 * @note Awaiter types only have access to the suspended task through this type
 */
class task_ref final {
 public:
  constexpr task_ref() noexcept : handle_(nullptr) {}

  template <typename T>
  explicit constexpr task_ref(task<T> & task) noexcept
      : handle_(task.handle_) {}

  explicit constexpr task_ref(std::coroutine_handle<> handle) noexcept
      : handle_(handle) {}

 public:
  explicit constexpr operator bool() const noexcept {
    return static_cast<bool>(handle_);
  }

  constexpr void operator()() { resume(); }

  constexpr auto operator<=>(task_ref const & that) const noexcept
      -> bool = default;

 public:
  /**
   * @note True only if task reference has a valid and done handle
   */
  constexpr auto done() const noexcept -> bool {
    return handle_ && handle_.done();
  };

  constexpr void resume() {
    if (!handle_) [[unlikely]] {
      throw std::runtime_error("task_ref::resume(): no state");
    }
    if (handle_.done()) [[unlikely]] {
      throw std::runtime_error("task_ref::resume(): already done");
    }

    handle_.resume();
  }

 private:
  std::coroutine_handle<> handle_;
};

}  // namespace argo
