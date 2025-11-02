#pragma once

#include <concepts>
#include <memory>
#include <type_traits>
#include <utility>

namespace sandbox::utils {

/**
 * @brief Type-erased pimpl with value semantics
 */
class type_erased_pimpl final {
 public:
  template <typename T, typename... Args>
    requires(!std::is_array_v<T>)
  constexpr explicit type_erased_pimpl(std::in_place_type_t<T>,
                                       Args&&... args) noexcept(false)
    requires(std::constructible_from<T, Args...>)
      : delete_([](void* ptr) { delete static_cast<T*>(ptr); }),
        clone_([](const void* ptr) -> void* {
          return std::make_unique<T>(*static_cast<const T*>(ptr)).release();
        }),
        ptr_(new T(std::forward<Args>(args)...)) {}

  template <typename T>
  constexpr explicit type_erased_pimpl(T value) noexcept(false)
      : type_erased_pimpl(std::in_place_type<T>, std::move(value)) {}

  constexpr type_erased_pimpl(const type_erased_pimpl& that) noexcept(false) {
    *this = that;
  }

  constexpr type_erased_pimpl& operator=(
      const type_erased_pimpl& that) noexcept(false) {
    delete_ = that.delete_;
    clone_ = that.clone_;
    ptr_ = that.clone_(that.ptr_);
    return *this;
  }

  constexpr type_erased_pimpl(type_erased_pimpl&& that) noexcept {
    *this = std::move(that);
  }

  constexpr type_erased_pimpl& operator=(type_erased_pimpl&& that) noexcept {
    delete_ = std::exchange(that.delete_, nullptr);
    clone_ = std::exchange(that.clone_, nullptr);
    ptr_ = std::exchange(that.ptr_, nullptr);
    return *this;
  }

  constexpr ~type_erased_pimpl() noexcept {
    if (!valueless_after_move()) {
      delete_(ptr_);
    }
  }

 public:
  template <typename T>
  constexpr type_erased_pimpl& operator=(T value) noexcept(false) {
    new (this) type_erased_pimpl(std::move(value));
    return *this;
  }

 public:
  constexpr bool valueless_after_move() const noexcept { return !delete_; }

 public:
  template <typename T>
  constexpr T& get() noexcept {
    return *static_cast<T*>(ptr_);
  }

  template <typename T>
  constexpr const T& get() const noexcept {
    return *static_cast<T*>(ptr_);
  }

  template <typename T>
  constexpr T value() const noexcept
    requires std::is_copy_constructible_v<T>
  {
    return *static_cast<T*>(ptr_);
  }

 private:
  using clone_t = void* (*)(const void*);
  using delete_t = void (*)(void*);

 private:
  delete_t delete_;
  clone_t clone_;
  void* ptr_;
};

}  // namespace sandbox::utils
