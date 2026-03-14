#pragma once

#include <wip/utils/mixins.hpp>

#include <chrono>
#include <coroutine>
#include <exception>
#include <iostream>
#include <optional>

namespace wip::async {

template <typename T>
class task final : public utils::non_copyable {
 public:
  class promise_type final : public utils::non_copyable,
                             public utils::non_movable {
   public:
    constexpr auto get_return_object() noexcept {
      return task<T>(std::coroutine_handle<promise_type>::from_promise(*this));
    }

    promise_type() {
      std::cerr
          << std::chrono::system_clock::now() << " - task::promise_type("
          << std::coroutine_handle<promise_type>::from_promise(*this).address()
          << ")" << std::endl;
    }
    ~promise_type() {
      task_->handle_ = nullptr;
      std::cerr
          << std::chrono::system_clock::now() << " - ~task::promise_type("
          << std::coroutine_handle<promise_type>::from_promise(*this).address()
          << ")" << std::endl;
    }

    constexpr auto initial_suspend() const noexcept {
      return std::suspend_always{};
    }

    constexpr auto final_suspend() noexcept {
      std::cerr
          << std::chrono::system_clock::now() << " - final_suspend("
          << std::coroutine_handle<promise_type>::from_promise(*this).address()
          << ")" << std::endl;
      struct aw_final : public utils::non_copyable, public utils::non_movable {
        bool await_ready() const noexcept {
          bool ready = !parent_ || parent_.done();
          std::cerr << std::chrono::system_clock::now() << " - aw_final::ready("
                    << handle_.address() << ") -> "
                    << (ready ? "true" : "false") << std::endl;
          return ready;
        }

        auto await_suspend(std::coroutine_handle<>) const noexcept {
          std::cerr << std::chrono::system_clock::now()
                    << " - aw_final::suspend_continue(" << handle_.address()
                    << ") -> "
                    << (parent_ ? parent_ : std::noop_coroutine()).address()
                    << std::endl;
          return parent_ ? parent_ : std::noop_coroutine();
        }

        auto await_resume() const noexcept {
          std::cerr << std::chrono::system_clock::now()
                    << " - aw_final::resume(" << handle_.address() << ")"
                    << std::endl;
        }

        std::coroutine_handle<> handle_ = nullptr;
        std::coroutine_handle<> parent_ = nullptr;
      };
      return aw_final{
          .handle_ = std::coroutine_handle<promise_type>::from_promise(*this),
          .parent_ = parent_};
    }

    constexpr void return_value(T&& value) noexcept {
      std::cerr
          << std::chrono::system_clock::now() << " - return_value("
          << std::coroutine_handle<promise_type>::from_promise(*this).address()
          << ", " << value << ")" << std::endl;
      value_.emplace(std::move(value));
    }

    constexpr void unhandled_exception() noexcept {
      exception_ = std::current_exception();

      try {
        std::rethrow_exception(std::current_exception());
      } catch (std::exception& e) {
        std::cerr << std::chrono::system_clock::now() << " - exception("
                  << std::coroutine_handle<promise_type>::from_promise(*this)
                         .address()
                  << "): " << e.what() << std::endl;
        throw;
      }
    }

    void set_parent(std::coroutine_handle<> parent) { parent_ = parent; }
    void set_task(task* task) { task_ = task; }
    task* get_task() const { return task_; }

   public:
    constexpr const T& get() const {
      if (value_.has_value()) {
        return *value_;
      } else if (exception_) {
        std::rethrow_exception(exception_);
      } else {
        throw std::runtime_error("promise_type value is not ready");
      }
    }

   private:
    std::optional<T> value_;
    std::exception_ptr exception_;
    std::coroutine_handle<> parent_;
    task* task_;
  };

 public:
  constexpr explicit task(std::coroutine_handle<promise_type> handle) noexcept
      : handle_(std::move(handle)) {
    handle.promise().set_task(this);
    std::cerr << std::chrono::system_clock::now() << " - task("
              << get_handle().address() << ")" << std::endl;
  }

  constexpr task(task&& that) noexcept
      : handle_(std::exchange(that.handle_, nullptr)) {
    if (handle_) {
      handle_.promise().set_task(this);
    }
  }

  constexpr task& operator=(task&& that) = delete;

  constexpr ~task() noexcept {
    if (handle_) {
      std::cerr << std::chrono::system_clock::now() << " - ~task("
                << handle_.address() << ")" << std::endl;
      handle_.destroy();
    } else {
      std::cerr << std::chrono::system_clock::now() << " - ~task(nullptr)"
                << std::endl;
    }
  }

 public:
  constexpr std::coroutine_handle<promise_type> get_handle() const noexcept {
    return handle_;
  }

  constexpr void resume() const {
    if (!handle_ || handle_.done()) {
      throw std::runtime_error("resume() called on a done handle");
    }
    handle_.resume();
  }

  constexpr const T& get() const { return handle_.promise().get(); }

 private:
  std::coroutine_handle<promise_type> handle_;
};

}  // namespace wip::async
