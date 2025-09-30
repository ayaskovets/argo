#pragma once

#include <memory>
#include <unordered_map>

namespace core::utils {

/**
 * @brief Simple leak detecting allocator. If Shared=true allocations are stored
 * on per-allocator-class basis rather than per-allocator-variable
 */
template <typename T, bool Shared = false>
class leak_detector_allocator : public std::allocator<T> {
 public:
  using std::allocator<T>::allocator;

 public:
  constexpr T* allocate(std::size_t n) {
    T* ptr = std::allocator<T>::allocate(n);
    if constexpr (Shared) {
      static_allocations_by_ptr_[ptr] += n;
    } else {
      allocations_by_ptr_[ptr] += n;
    }
    return ptr;
  }

  constexpr std::allocation_result<T*> allocate_at_least(std::size_t n) {
    return std::allocation_result<T*>{.ptr = allocate(n), .count = n};
  }

  constexpr void deallocate(T* ptr, std::size_t n) {
    if constexpr (Shared) {
      if ((static_allocations_by_ptr_.at(ptr) -= n) == 0) {
        static_allocations_by_ptr_.erase(ptr);
      }
    } else {
      if ((allocations_by_ptr_.at(ptr) -= n) == 0) {
        allocations_by_ptr_.erase(ptr);
      }
    }
    std::allocator<T>::deallocate(ptr, n);
  }

 public:
  template <typename To>
  class rebind {
   public:
    typedef leak_detector_allocator<To, Shared> other;
  };

 public:
  constexpr void clear() noexcept
    requires(Shared)
  {
    static_allocations_by_ptr_.clear();
  }

 public:
  constexpr bool is_clean() const noexcept {
    if constexpr (Shared) {
      return static_allocations_by_ptr_.empty();
    } else {
      return allocations_by_ptr_.empty();
    }
  }

 private:
  std::unordered_map<void*, std::size_t> allocations_by_ptr_;
  static inline std::unordered_map<void*, std::size_t>
      static_allocations_by_ptr_;
};

}  // namespace core::utils
